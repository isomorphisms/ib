#!/usr/bin/env python3
"""Hosted browser observer. Not the installed IB app; secrets never in receipts."""
import argparse
import os
import pathlib
import shutil
import tempfile
import threading
import time

import psutil
from playwright.sync_api import sync_playwright
from fixture_server import (FixtureServer, FixtureState, ASSET_COUNT, ASSET_BYTES,
                            private_disk_usage, SYNTHETIC_USER, call_journal)

def login(page, url, state):
    page.goto(url + "/admin", wait_until="domcontentloaded")
    if "/login" in page.url:
        page.locator("#account").fill(SYNTHETIC_USER)
        page.locator("#password").fill(state.password)
        page.get_by_role("button", name="Sign in").click()
    page.locator("#useful-table").wait_for(state="visible")
    page.locator("#create").wait_for(state="visible")


def browser_rss(browser):
    try:
        process = psutil.Process(browser._impl_obj._connection._transport._proc.pid)
        return sum(p.memory_info().rss for p in [process] + process.children(recursive=True)
                   if p.is_running())
    except (AttributeError, psutil.Error):
        return None


def run(driver, evidence):
    state = FixtureState()
    server = FixtureServer(state)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://127.0.0.1:{server.server_address[1]}"
    measurements = {}
    try:
        with tempfile.TemporaryDirectory(prefix="ib-admin-case-") as private:
            os.chmod(private, 0o700)
            disk_before = private_disk_usage(private)
            with sync_playwright() as p:
                chromium_path = os.environ.get("IB_CHROMIUM") or shutil.which("chromium") or p.chromium.executable_path
                browser = p.chromium.launch(executable_path=chromium_path, headless=True,
                    args=["--no-sandbox", "--disable-dev-shm-usage"])
                context = browser.new_context()
                page = context.new_page()
                login(page, base, state)
                page.wait_for_load_state("load", timeout=20000)
                with state.lock:
                    before_heavy_navigation = state.body_bytes
                started = time.monotonic()
                page.goto(base + "/admin", wait_until="domcontentloaded")
                page.locator("#useful-table").wait_for(state="visible")
                page.locator("#create").wait_for(state="visible")
                measurements["first_useful_ms"] = int((time.monotonic() - started) * 1000)
                with state.lock:
                    measurements["bytes_when_useful"] = state.body_bytes - before_heavy_navigation
                call_journal(driver, private, "taskSuccess", "checkpoint", "OrdinaryOne")
                call_journal(driver, private, "taskSuccess", "prepare", "opSuccess")
                sample_before = browser_rss(browser)
                # A real second browser tab exercises an interruption without
                # claiming an Android onPause or OS-level app switch.
                other = context.new_page()
                other.goto(base + "/verification")
                assert other.locator("#verification").is_visible()
                other.close()
                page.locator("#label").fill("OrdinaryOne")
                # Browser-owner checkpoint is already durable; the DOM field
                # has no authority to make a second mutation on its own.
                assert "dispatch_allowed=1" in call_journal(driver, private, "taskSuccess", "can-dispatch")
                call_journal(driver, private, "taskSuccess", "mark-may-have-applied")
                page.locator("#create").evaluate("(button) => button.dataset.operation = 'opSuccess'")
                page.locator("#create").click()
                page.wait_for_function("document.getElementById('status').textContent === 'Created'", timeout=10000)
                # Synthetic key observed ONLY in transient in-memory DOM.
                rendered_key = page.locator("#once-only-key").text_content()
                with state.lock:
                    assert rendered_key == state.created["opSuccess"]["one_time_secret"]
                assert rendered_key.startswith("synthetic_")
                # Full irrelevant body transfer is forced, but finishes after
                # first useful progress rather than gating interaction.
                page.wait_for_load_state("load", timeout=20000)
                with state.lock:
                    measurements["total_served_body_bytes"] = state.body_bytes - before_heavy_navigation
                    measurements["served_responses"] = state.responses
                measurements["browser_rss_sample_bytes"] = sample_before
                assert measurements["bytes_when_useful"] < measurements["total_served_body_bytes"]
                assert measurements["total_served_body_bytes"] - measurements["bytes_when_useful"] >= ASSET_BYTES
                assert state.attempts("opSuccess") == 1
                call_journal(driver, private, "taskSuccess", "observe-applied", "opSuccess")
                page.reload(wait_until="domcontentloaded")
                assert not page.locator("#once-only-key").text_content()
                assert call_journal(driver, private, "taskSuccess", "can-dispatch", expected=4) == ""
                browser.close()

                # Separate operation. Server executes POST, loses reply. Only
                # durable MAY_HAVE_APPLIED may survive renderer/process loss.
                call_journal(driver, private, "taskAmbiguous", "checkpoint", "OrdinaryTwo")
                call_journal(driver, private, "taskAmbiguous", "prepare", "opAmbiguous")
                browser2 = p.chromium.launch(executable_path=chromium_path, headless=True,
                    args=["--no-sandbox", "--disable-dev-shm-usage"])
                context2 = browser2.new_context()
                page2 = context2.new_page()
                login(page2, base, state)
                page2.locator("#label").fill("OrdinaryTwo")
                call_journal(driver, private, "taskAmbiguous", "mark-may-have-applied")
                page2.locator("#create").evaluate("(button) => button.dataset.operation = 'opAmbiguous'")
                page2.locator("#create").click()
                page2.get_by_text("Unknown outcome; do not retry").wait_for(timeout=10000)
                assert state.attempts("opAmbiguous") == 1
                browser2.close()
                # Entire browser process and its JavaScript heap are gone.
                # No cookie is persisted; fresh authentication is mandatory.
                assert "MAY_HAVE_APPLIED" in call_journal(driver, private, "taskAmbiguous", "inspect")
                assert call_journal(driver, private, "taskAmbiguous", "can-dispatch", expected=4) == ""
                assert call_journal(driver, private, "taskAmbiguous", "mark-may-have-applied", expected=5) == ""
                browser3 = p.chromium.launch(executable_path=chromium_path, headless=True,
                    args=["--no-sandbox", "--disable-dev-shm-usage"])
                context3 = browser3.new_context()
                page3 = context3.new_page()
                login(page3, base, state)
                status = page3.evaluate("async (u) => (await (await fetch(u)).json())",
                    base + "/api/status?operation=opAmbiguous")
                assert status == {"applied": True, "operation": "opAmbiguous"}
                # Only the authenticated, ID-bound read-after-write observer
                # is allowed to mark ordinary completion. Not a second POST.
                call_journal(driver, private, "taskAmbiguous", "observe-applied", status["operation"])
                browser3.close()
                assert "OBSERVED_APPLIED" in call_journal(driver, private, "taskAmbiguous", "inspect")
                assert state.attempts("opAmbiguous") == 1
                assert state.attempts("opSuccess") == 1

            measurements["journal_disk_growth_bytes"] = private_disk_usage(private) - disk_before
            records = b"".join(path.read_bytes() for path in pathlib.Path(private).rglob("*.event"))
            with state.lock:
                for entry in state.created.values():
                    assert entry["one_time_secret"].encode() not in records
                assert state.password.encode() not in records
                assert state.session.encode() not in records
            assert b"synthetic_" not in records
            assert len(list(pathlib.Path(private).rglob("*.event"))) == 8

        if evidence:
            out = pathlib.Path(evidence)
            out.parent.mkdir(parents=True, exist_ok=True)
            lines = [
                "schema\tib-admin-hosted-acceptance-v1",
                "fixture\tloopback-synthetic-credentials-only",
                "rendering_authority\theadless-chromium-not-installed-ib",
                "native_policy\tib_admin_transaction.c",
                "first_useful_ms\t" + str(measurements["first_useful_ms"]),
                "served_body_bytes_when_useful\t" + str(measurements["bytes_when_useful"]),
                "served_body_bytes_total\t" + str(measurements["total_served_body_bytes"]),
                "served_responses\t" + str(measurements["served_responses"]),
                "browser_rss_sample_bytes\t" + (str(measurements["browser_rss_sample_bytes"])
                    if measurements["browser_rss_sample_bytes"] is not None else "not-measured"),
                "renderer_peak_pss_bytes\tnot-measured",
                "journal_disk_growth_bytes\t" + str(measurements["journal_disk_growth_bytes"]),
                "app_switch\tsecond-browser-tab-only-not-android-app-switch",
                "renderer_relaunch\tnew-chromium-process",
                "success_post_count\t1",
                "ambiguous_post_count\t1",
                "ambiguous_replay\trefused",
                "retained_ordinary_status\tread-after-write-and-reopen",
                "one_time_secret\tnot-retained",
                "authenticated_google_or_youtube\tNOT_RUN",
                "android_art\tNOT_RUN",
                "physical_android\tNOT_RUN",
                "IB_native_viewport\tNOT_RUN",
            ]
            out.write_text("\n".join(lines) + "\n")
        return measurements
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--evidence")
    args = parser.parse_args()
    measurements = run(args.driver, args.evidence)
    print("PASS actual Chromium fixture interaction, interrupted return, committed C action journal,")
    print("PASS one-time synthetic secret excluded from journal; uncertain POST never replayed")
    print("first_useful_ms", measurements["first_useful_ms"])
    print("served_body_bytes_total", measurements["total_served_body_bytes"])
    print("journal_disk_growth_bytes", measurements["journal_disk_growth_bytes"])
    print("NOTE no installed IB Activity or Android/Google qualification")


if __name__ == "__main__":
    main()
