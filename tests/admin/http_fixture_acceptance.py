#!/usr/bin/env python3
"""Host HTTP/transaction qualification; does NOT assert a browser/Android pass."""
import argparse
import concurrent.futures
import http.cookiejar
import http.client
import json
import os
import pathlib
import resource
import socket
import subprocess
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request

from fixture_server import (FixtureServer, FixtureState, ASSET_COUNT,
                                       ASSET_BYTES, private_disk_usage, SYNTHETIC_USER,
                                       call_journal)


def visit(opener, url, data=None, headers=None):
    req = urllib.request.Request(url, data=data, headers=headers or {})
    with opener.open(req, timeout=15) as response:
        return response.read()


def auth(base, fixture):
    cookiejar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cookiejar))
    # This login is explicitly synthetic and entirely loopback. Never log it.
    visit(opener, base + '/login')
    body = urllib.parse.urlencode({'account': SYNTHETIC_USER, 'password': fixture.password}).encode()
    visit(opener, base + '/login', body, {'Content-Type': 'application/x-www-form-urlencoded'})
    return opener


def submit(opener, base, label, operation):
    body = json.dumps({'label': label, 'operation': operation}).encode()
    return visit(opener, base + '/api/create', body, {'Content-Type': 'application/json'})


def run(driver, evidence):
    fixture = FixtureState()
    server = FixtureServer(fixture)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = 'http://127.0.0.1:' + str(server.server_address[1])
    try:
        with tempfile.TemporaryDirectory(prefix='ib-admin-http-test-') as root:
            os.chmod(root, 0o700)
            disk_before = private_disk_usage(root)
            unauthed = urllib.request.build_opener()
            try:
                visit(unauthed, base + '/api/status?operation=opSuccess')
                raise AssertionError('Unauthenticated status was admitted')
            except urllib.error.HTTPError as error:
                assert error.code == 401
            opener = auth(base, fixture)
            stamp = time.monotonic()
            html = visit(opener, base + '/admin')
            useful_ms = int((time.monotonic() - stamp) * 1000)
            assert b'id=\'useful-table\'' in html and b'id=\'create\'' in html
            bytes_at_useful = fixture.body_bytes
            with concurrent.futures.ThreadPoolExecutor(max_workers=ASSET_COUNT) as executor:
                futures = [executor.submit(visit, opener, base + f'/asset/{n}')
                           for n in range(ASSET_COUNT)]
                call_journal(driver, root, 'taskSuccess', 'checkpoint', 'OrdinaryOne')
                call_journal(driver, root, 'taskSuccess', 'prepare', 'opSuccess')
                call_journal(driver, root, 'taskSuccess', 'mark-may-have-applied')
                success = json.loads(submit(opener, base, 'OrdinaryOne', 'opSuccess'))
                assert success['key'].startswith('synthetic_')
                assert fixture.attempts('opSuccess') == 1
                call_journal(driver, root, 'taskSuccess', 'observe-applied', 'opSuccess')
                assert all(len(f.result()) == ASSET_BYTES for f in futures)
            for _ in range(100):
                if fixture.body_bytes >= bytes_at_useful + ASSET_BYTES * ASSET_COUNT:
                    break
                time.sleep(0.02)
            assert fixture.body_bytes >= bytes_at_useful + ASSET_BYTES * ASSET_COUNT
            assert 'OBSERVED_APPLIED' in call_journal(driver, root, 'taskSuccess', 'inspect')
            assert call_journal(driver, root, 'taskSuccess', 'can-dispatch', expected=4) == ''

            # Verification detour and a fresh HTTP client follow. This is NOT
            # Android onPause/onResume; the URL fetch simply exercises fixture.
            assert b'Verification interruption' in visit(opener, base + '/verification')
            call_journal(driver, root, 'taskAmbiguous', 'checkpoint', 'OrdinaryTwo')
            call_journal(driver, root, 'taskAmbiguous', 'prepare', 'opAmbiguous')
            call_journal(driver, root, 'taskAmbiguous', 'mark-may-have-applied')
            try:
                submit(opener, base, 'OrdinaryTwo', 'opAmbiguous')
                raise AssertionError('Ambiguous POST unexpectedly returned')
            except (http.client.RemoteDisconnected, ConnectionResetError,
                    urllib.error.URLError, OSError):
                pass
            assert fixture.attempts('opAmbiguous') == 1
            assert 'MAY_HAVE_APPLIED' in call_journal(driver, root, 'taskAmbiguous', 'inspect')
            assert call_journal(driver, root, 'taskAmbiguous', 'mark-may-have-applied', expected=5) == ''
            assert call_journal(driver, root, 'taskAmbiguous', 'can-dispatch', expected=4) == ''
            fresh = auth(base, fixture)
            remote = json.loads(visit(fresh, base + '/api/status?operation=opAmbiguous'))
            assert remote == {'applied': True, 'operation': 'opAmbiguous'}
            call_journal(driver, root, 'taskAmbiguous', 'observe-applied', remote['operation'])
            assert 'OBSERVED_APPLIED' in call_journal(driver, root, 'taskAmbiguous', 'inspect')
            assert fixture.attempts('opAmbiguous') == 1
            disk_growth = private_disk_usage(root) - disk_before
            records = b''.join(x.read_bytes() for x in pathlib.Path(root).rglob('*.event'))
            assert len(list(pathlib.Path(root).rglob('*.event'))) == 8
            with fixture.lock:
                for value in fixture.created.values():
                    assert value['one_time_secret'].encode() not in records
                assert fixture.session.encode() not in records
                assert fixture.password.encode() not in records
            assert b'synthetic_' not in records
            counts = {'first_useful_http_ms': useful_ms,
                      'body_bytes_at_useful_http': bytes_at_useful,
                      'body_bytes_total': fixture.body_bytes,
                      'responses_total': fixture.responses,
                      'journal_disk_growth_bytes': disk_growth,
                      'fixture_host_peak_rss_kib': resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                      'success_post_count': fixture.attempts('opSuccess'),
                      'ambiguous_post_count': fixture.attempts('opAmbiguous')}
        if evidence:
            out = pathlib.Path(evidence)
            out.parent.mkdir(parents=True, exist_ok=True)
            rows = ['schema\tib-admin-http-host-v1',
                    'host\tlocal-http-client-not-renderer',
                    'authentication\tsynthetic-loopback',
                    'retained_state\tnative-c-journal',
                    'actual_browser_ui\tNOT_RUN',
                    'android_art\tNOT_RUN',
                    'google_or_youtube\tNOT_RUN',
                    'physical_device\tNOT_RUN',
                    'browser_network_bytes\tnot-measured',
                    'peak_browser_pss\tnot-measured',
                    'one_time_key_retained\tfalse',
                    'ambiguous_post_replayed\tfalse']
            rows += [f'{k}\t{v}' for k, v in counts.items()]
            out.write_text('\n'.join(rows) + '\n')
        return counts
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--driver', required=True)
    parser.add_argument('--evidence')
    args = parser.parse_args()
    result = run(args.driver, args.evidence)
    print('PASS actual loopback HTTP, authentication, slow assets, one-time key,')
    print('PASS C journal recovery, ambiguous POST non-replay, no secrets in journal')
    print('first_useful_http_ms', result['first_useful_http_ms'])
    print('body_bytes_total', result['body_bytes_total'])
    print('journal_disk_growth_bytes', result['journal_disk_growth_bytes'])
    print('fixture_host_peak_rss_kib', result['fixture_host_peak_rss_kib'])
    print('NOTE browser-owned UI and Android application NOT_RUN')

if __name__ == '__main__':
    main()
