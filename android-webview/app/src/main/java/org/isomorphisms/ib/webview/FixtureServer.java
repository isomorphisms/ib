package org.isomorphisms.ib.webview;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.nio.charset.StandardCharsets;
import java.util.UUID;

final class FixtureServer implements AutoCloseable {
    private final String session_value = UUID.randomUUID().toString();
    private volatile boolean running;
    private ServerSocket server_socket;
    private Thread accept_thread;

    void start() throws IOException {
        if (running) {
            return;
        }
        server_socket = new ServerSocket();
        server_socket.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
        running = true;
        accept_thread = new Thread(this::accept_loop, "ib-fixture-http");
        accept_thread.setDaemon(true);
        accept_thread.start();
    }

    String start_url() {
        return base_url() + "/start";
    }

    String form_url() {
        return base_url() + "/form";
    }

    String probe_url() {
        return base_url() + "/probe";
    }

    private String base_url() {
        if (server_socket == null) {
            throw new IllegalStateException("fixture server has not started");
        }
        return "http://127.0.0.1:" + server_socket.getLocalPort();
    }

    private void accept_loop() {
        while (running) {
            try {
                Socket socket = server_socket.accept();
                serve(socket);
            } catch (SocketException exception) {
                if (running) {
                    throw new IllegalStateException("fixture server socket failed", exception);
                }
            } catch (IOException exception) {
                if (running) {
                    throw new IllegalStateException("fixture server failed", exception);
                }
            }
        }
    }

    private void serve(Socket socket) {
        try (socket;
             BufferedReader reader = new BufferedReader(
                 new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))) {
            String request_line = reader.readLine();
            if (request_line == null || request_line.isEmpty()) {
                return;
            }

            String cookie = null;
            String line;
            while ((line = reader.readLine()) != null && !line.isEmpty()) {
                int colon = line.indexOf(':');
                if (colon > 0 && line.substring(0, colon).trim().equalsIgnoreCase("Cookie")) {
                    cookie = line.substring(colon + 1).trim();
                }
            }

            String[] request_parts = request_line.split(" ", 3);
            if (request_parts.length < 2) {
                respond(socket, 400, "Bad Request", "text/plain; charset=utf-8", "bad request", null, null);
                return;
            }

            String target = request_parts[1];
            int query = target.indexOf('?');
            String path = query >= 0 ? target.substring(0, query) : target;
            boolean authenticated = cookie != null && cookie.contains("ib_session=" + session_value);

            switch (path) {
                case "/start" -> respond(
                    socket,
                    302,
                    "Found",
                    "text/plain; charset=utf-8",
                    "",
                    "/form",
                    "ib_session=" + session_value + "; Path=/; HttpOnly; SameSite=Lax"
                );
                case "/form" -> {
                    if (!authenticated) {
                        respond(socket, 401, "Unauthorized", "text/html; charset=utf-8", unauthorized_page(), null, null);
                    } else {
                        respond(socket, 200, "OK", "text/html; charset=utf-8", form_page(), null, null);
                    }
                }
                case "/probe" -> respond(
                    socket,
                    authenticated ? 200 : 401,
                    authenticated ? "OK" : "Unauthorized",
                    "text/plain; charset=utf-8",
                    authenticated ? "authenticated" : "unauthenticated",
                    null,
                    null
                );
                default -> respond(socket, 404, "Not Found", "text/plain; charset=utf-8", "not found", null, null);
            }
        } catch (IOException ignored) {
            // The fixture exists only to provide deterministic local HTTP behavior.
        }
    }

    private static void respond(
        Socket socket,
        int code,
        String reason,
        String content_type,
        String body,
        String location,
        String set_cookie
    ) throws IOException {
        byte[] body_bytes = body.getBytes(StandardCharsets.UTF_8);
        StringBuilder headers = new StringBuilder();
        headers.append("HTTP/1.1 ").append(code).append(' ').append(reason).append("\r\n");
        headers.append("Content-Type: ").append(content_type).append("\r\n");
        headers.append("Content-Length: ").append(body_bytes.length).append("\r\n");
        headers.append("Cache-Control: no-store\r\n");
        headers.append("Connection: close\r\n");
        if (location != null) {
            headers.append("Location: ").append(location).append("\r\n");
        }
        if (set_cookie != null) {
            headers.append("Set-Cookie: ").append(set_cookie).append("\r\n");
        }
        headers.append("\r\n");

        OutputStream output = socket.getOutputStream();
        output.write(headers.toString().getBytes(StandardCharsets.US_ASCII));
        output.write(body_bytes);
        output.flush();
    }

    private static String form_page() {
        return "<!doctype html>"
            + "<html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            + "<title>IB protected transaction fixture</title>"
            + "<style>body{font-family:sans-serif;padding:18px;line-height:1.4}label{display:block;margin:18px 0}"
            + "input{display:block;width:95%;font-size:18px;padding:8px}.note{font-size:14px}</style></head><body>"
            + "<h2>Protected transaction fixture</h2>"
            + "<p class=\"note\">The ordinary field may be reconstructed. The synthetic secret must never be checkpointed.</p>"
            + "<label>Ordinary field<input id=\"ordinary_field\" autocomplete=\"off\"></label>"
            + "<label>Synthetic secret<input id=\"secret_field\" type=\"password\" autocomplete=\"off\"></label>"
            + "<p id=\"heap_marker\" class=\"note\"></p>"
            + "<script>"
            + "window.heap_canary=(self.crypto&&crypto.randomUUID)?crypto.randomUUID():String(Date.now())+Math.random();"
            + "document.getElementById('heap_marker').textContent='renderer canary '+window.heap_canary;"
            + "</script></body></html>";
    }

    private static String unauthorized_page() {
        return "<!doctype html><html><head><title>unauthenticated</title></head>"
            + "<body><h2>Session missing</h2><p>The authenticated fixture session did not survive.</p></body></html>";
    }

    @Override
    public void close() {
        running = false;
        if (server_socket != null) {
            try {
                server_socket.close();
            } catch (IOException ignored) {
                // Closing the server is best-effort during Activity teardown.
            }
        }
    }
}
