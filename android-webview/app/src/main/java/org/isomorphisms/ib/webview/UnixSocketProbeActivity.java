package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.net.Credentials;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Build;
import android.os.Bundle;
import android.os.Process;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.Closeable;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class UnixSocketProbeActivity extends Activity {
    private static final String IB_ABSTRACT_NAME = "ib-longview-86";
    private static final String TERMUX_ABSTRACT_NAME = "termux-longview-86";
    private static final String MISSING_ABSTRACT_NAME = "ib-longview-86-no-listener";
    private static final String DEFAULT_TERMUX_PATH =
        "/data/data/com.termux/files/usr/tmp/ib-longview-86.sock";
    private static final byte[] PING = "ping\n".getBytes(StandardCharsets.US_ASCII);
    private static final byte[] PONG = "pong\n".getBytes(StandardCharsets.US_ASCII);

    private final ExecutorService executor = Executors.newCachedThreadPool();
    private final Object receipt_lock = new Object();
    private final Object listener_lock = new Object();

    private TextView receipt_view;
    private EditText termux_path_view;
    private File receipt_file;
    private ListenerHandle active_listener;

    @Override
    protected void onCreate(Bundle saved_instance_state) {
        super.onCreate(saved_instance_state);

        receipt_file = new File(getFilesDir(), "uds-probe-receipt.tsv");

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        column.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("IB issue #86 — cross-UID Unix socket probe");
        title.setTextSize(20);
        column.addView(title);

        TextView names = new TextView(this);
        names.setText(
            "IB abstract: " + IB_ABSTRACT_NAME + "\n"
                + "Termux abstract: " + TERMUX_ABSTRACT_NAME + "\n"
                + "IB private path: " + ib_private_path() + "\n"
                + "Termux private path:"
        );
        column.addView(names);

        termux_path_view = new EditText(this);
        termux_path_view.setSingleLine(true);
        termux_path_view.setText(DEFAULT_TERMUX_PATH);
        column.addView(termux_path_view);

        add_button(column, "IB listen abstract once",
            view -> start_listener(
                LocalSocketAddress.Namespace.ABSTRACT,
                IB_ABSTRACT_NAME,
                false
            ));
        add_button(column, "IB listen abstract + hold",
            view -> start_listener(
                LocalSocketAddress.Namespace.ABSTRACT,
                IB_ABSTRACT_NAME,
                true
            ));
        add_button(column, "IB connect to Termux abstract once",
            view -> start_client(
                LocalSocketAddress.Namespace.ABSTRACT,
                TERMUX_ABSTRACT_NAME,
                false,
                "termux-abstract"
            ));
        add_button(column, "IB connect to Termux abstract + hold",
            view -> start_client(
                LocalSocketAddress.Namespace.ABSTRACT,
                TERMUX_ABSTRACT_NAME,
                true,
                "termux-abstract"
            ));
        add_button(column, "IB listen on IB-private pathname",
            view -> start_listener(
                LocalSocketAddress.Namespace.FILESYSTEM,
                ib_private_path(),
                false
            ));
        add_button(column, "IB connect to Termux-private pathname",
            view -> start_client(
                LocalSocketAddress.Namespace.FILESYSTEM,
                termux_path_view.getText().toString().trim(),
                false,
                "termux-private-path"
            ));
        add_button(column, "Check abstract name with no listener",
            view -> start_client(
                LocalSocketAddress.Namespace.ABSTRACT,
                MISSING_ABSTRACT_NAME,
                false,
                "expected-no-listener"
            ));
        add_button(column, "Stop current IB listener", view -> stop_active_listener());
        add_button(column, "Copy receipt", view -> copy_receipt());
        add_button(column, "Kill IB host", view -> {
            append_receipt(
                "operation=kill-ib-host\tpid=" + Process.myPid()
                    + "\tactive_listener=" + (active_listener != null)
            );
            Process.killProcess(Process.myPid());
        });

        receipt_view = new TextView(this);
        receipt_view.setTextIsSelectable(true);
        column.addView(receipt_view);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(column);
        setContentView(scroll);

        append_receipt(
            "run-start=" + System.currentTimeMillis()
                + "\tsource_head=" + BuildConfig.IB_SOURCE_HEAD
                + "\tandroid=" + Build.VERSION.RELEASE
                + "\tsdk=" + Build.VERSION.SDK_INT
                + "\tpackage=" + getPackageName()
                + "\tpid=" + Process.myPid()
                + "\tuid=" + Process.myUid()
        );
        append_receipt(
            "ib_abstract=" + IB_ABSTRACT_NAME
                + "\ttermux_abstract=" + TERMUX_ABSTRACT_NAME
                + "\tib_private_path=" + ib_private_path()
        );
    }

    private void add_button(
        LinearLayout parent,
        String label,
        View.OnClickListener listener
    ) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(listener);
        parent.addView(button);
    }

    private String ib_private_path() {
        return new File(getFilesDir(), "ib-longview-86.sock").getAbsolutePath();
    }

    private void start_listener(
        LocalSocketAddress.Namespace namespace,
        String address,
        boolean hold
    ) {
        stop_active_listener();
        append_receipt(
            "operation=listen-start\tnamespace=" + namespace_text(namespace)
                + "\taddress=" + address
                + "\thold=" + hold
        );

        executor.execute(() -> {
            ListenerHandle handle = null;
            try {
                handle = open_listener(namespace, address);
                synchronized (listener_lock) {
                    active_listener = handle;
                }
                append_receipt(
                    "operation=listen-ready\tnamespace=" + namespace_text(namespace)
                        + "\taddress=" + address
                );

                LocalSocket peer = handle.server.accept();
                try {
                    append_receipt(
                        "operation=accept\t" + peer_credentials(peer)
                    );
                    peer.setSoTimeout(hold ? 0 : 5000);
                    byte[] received = read_exact(peer.getInputStream(), PING.length);
                    if (!Arrays.equals(PING, received)) {
                        append_receipt(
                            "operation=exchange\trole=listener\tstatus=wrong-payload"
                        );
                        return;
                    }
                    OutputStream output = peer.getOutputStream();
                    output.write(PONG);
                    output.flush();
                    append_receipt(
                        "operation=exchange\trole=listener\tstatus=pass"
                    );
                    if (hold) {
                        wait_for_peer_close(peer, "listener");
                    }
                } finally {
                    close_quietly(peer);
                }
            } catch (IOException exception) {
                append_receipt(
                    "operation=listen\tstatus=error\tnamespace="
                        + namespace_text(namespace)
                        + "\taddress=" + address
                        + "\terror=" + one_line(exception)
                );
            } finally {
                if (handle != null) {
                    handle.close();
                }
                synchronized (listener_lock) {
                    if (active_listener == handle) {
                        active_listener = null;
                    }
                }
                append_receipt(
                    "operation=listen-end\tnamespace=" + namespace_text(namespace)
                        + "\taddress=" + address
                );
            }
        });
    }

    private ListenerHandle open_listener(
        LocalSocketAddress.Namespace namespace,
        String address
    ) throws IOException {
        if (namespace == LocalSocketAddress.Namespace.ABSTRACT) {
            return new ListenerHandle(new LocalServerSocket(address), null, null);
        }

        File socket_file = new File(address);
        if (socket_file.exists() && !socket_file.delete()) {
            throw new IOException("could not remove previous socket pathname");
        }

        LocalSocket backing_socket = new LocalSocket(LocalSocket.SOCKET_STREAM);
        boolean success = false;
        try {
            backing_socket.bind(new LocalSocketAddress(
                address,
                LocalSocketAddress.Namespace.FILESYSTEM
            ));
            LocalServerSocket server =
                new LocalServerSocket(backing_socket.getFileDescriptor());
            success = true;
            return new ListenerHandle(server, backing_socket, socket_file);
        } finally {
            if (!success) {
                close_quietly(backing_socket);
            }
        }
    }

    private void start_client(
        LocalSocketAddress.Namespace namespace,
        String address,
        boolean hold,
        String expectation
    ) {
        append_receipt(
            "operation=connect-start\tnamespace=" + namespace_text(namespace)
                + "\taddress=" + address
                + "\thold=" + hold
                + "\texpectation=" + expectation
        );

        executor.execute(() -> {
            try (LocalSocket socket = new LocalSocket(LocalSocket.SOCKET_STREAM)) {
                socket.connect(new LocalSocketAddress(address, namespace));
                socket.setSoTimeout(hold ? 0 : 5000);
                append_receipt(
                    "operation=connect\tstatus=connected\t" + peer_credentials(socket)
                );

                OutputStream output = socket.getOutputStream();
                output.write(PING);
                output.flush();
                byte[] received = read_exact(socket.getInputStream(), PONG.length);
                if (!Arrays.equals(PONG, received)) {
                    append_receipt(
                        "operation=exchange\trole=connector\tstatus=wrong-payload"
                    );
                    return;
                }
                append_receipt(
                    "operation=exchange\trole=connector\tstatus=pass"
                );
                if (hold) {
                    wait_for_peer_close(socket, "connector");
                }
            } catch (IOException exception) {
                append_receipt(
                    "operation=connect\tstatus=error\tnamespace="
                        + namespace_text(namespace)
                        + "\taddress=" + address
                        + "\texpectation=" + expectation
                        + "\terror=" + one_line(exception)
                );
            }
        });
    }

    private void wait_for_peer_close(LocalSocket socket, String role)
        throws IOException {
        append_receipt(
            "operation=hold\trole=" + role + "\tstatus=waiting-for-peer-close"
        );
        int value = socket.getInputStream().read();
        append_receipt(
            "operation=hold\trole=" + role
                + "\tstatus=" + (value < 0 ? "peer-eof" : "unexpected-byte-" + value)
        );
    }

    private static byte[] read_exact(InputStream input, int count)
        throws IOException {
        byte[] result = new byte[count];
        int offset = 0;
        while (offset < count) {
            int got = input.read(result, offset, count - offset);
            if (got < 0) {
                throw new EOFException(
                    "peer closed after " + offset + " of " + count + " bytes"
                );
            }
            offset += got;
        }
        return result;
    }

    private String peer_credentials(LocalSocket socket) {
        try {
            Credentials credentials = socket.getPeerCredentials();
            return "peer_pid=" + credentials.getPid()
                + "\tpeer_uid=" + credentials.getUid();
        } catch (IOException exception) {
            return "peer_credentials=unavailable"
                + "\tpeer_credentials_error=" + one_line(exception);
        }
    }

    private void stop_active_listener() {
        ListenerHandle handle;
        synchronized (listener_lock) {
            handle = active_listener;
            active_listener = null;
        }
        if (handle != null) {
            handle.close();
            append_receipt("operation=listen-stop\tstatus=requested");
        }
    }

    private void append_receipt(String line) {
        synchronized (receipt_lock) {
            try (FileOutputStream output =
                     new FileOutputStream(receipt_file, true)) {
                output.write((line + "\n").getBytes(StandardCharsets.UTF_8));
                output.getFD().sync();
            } catch (IOException ignored) {
            }
        }
        runOnUiThread(() -> {
            if (receipt_view != null) {
                receipt_view.setText(read_receipt());
            }
        });
    }

    private String read_receipt() {
        synchronized (receipt_lock) {
            if (!receipt_file.exists()) {
                return "";
            }
            try (FileInputStream input = new FileInputStream(receipt_file)) {
                byte[] bytes = new byte[(int) receipt_file.length()];
                int offset = 0;
                while (offset < bytes.length) {
                    int got = input.read(bytes, offset, bytes.length - offset);
                    if (got < 0) {
                        break;
                    }
                    offset += got;
                }
                return new String(bytes, 0, offset, StandardCharsets.UTF_8);
            } catch (IOException exception) {
                return "receipt-read-error=" + one_line(exception);
            }
        }
    }

    private void copy_receipt() {
        ClipboardManager clipboard =
            (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(
            ClipData.newPlainText("IB issue 86 receipt", read_receipt())
        );
    }

    private static String namespace_text(LocalSocketAddress.Namespace namespace) {
        return namespace == LocalSocketAddress.Namespace.ABSTRACT
            ? "abstract"
            : "filesystem";
    }

    private static String one_line(Throwable throwable) {
        String text = throwable.getClass().getSimpleName() + ": "
            + String.valueOf(throwable.getMessage());
        return text.replace('\n', ' ').replace('\r', ' ');
    }

    private static void close_quietly(Closeable closeable) {
        if (closeable == null) {
            return;
        }
        try {
            closeable.close();
        } catch (IOException ignored) {
        }
    }

    @Override
    protected void onDestroy() {
        stop_active_listener();
        executor.shutdownNow();
        super.onDestroy();
    }

    private static final class ListenerHandle implements Closeable {
        final LocalServerSocket server;
        final LocalSocket backing_socket;
        final File socket_file;

        ListenerHandle(
            LocalServerSocket server,
            LocalSocket backing_socket,
            File socket_file
        ) {
            this.server = server;
            this.backing_socket = backing_socket;
            this.socket_file = socket_file;
        }

        @Override
        public void close() {
            close_quietly(server);
            close_quietly(backing_socket);
            if (socket_file != null && socket_file.exists()) {
                socket_file.delete();
            }
        }
    }
}
