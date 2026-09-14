package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public final class PressureActivity extends Activity {
    private final List<byte[]> allocations = new ArrayList<>();
    private TextView status;
    private long allocated_bytes;

    @Override
    protected void onCreate(Bundle saved_instance_state) {
        super.onCreate(saved_instance_state);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int padding = dp(18);
        root.setPadding(padding, padding, padding, padding);

        TextView explanation = new TextView(this);
        explanation.setText(
            "This Activity runs in the :pressure process. Allocate memory here, then return to the protected WebView."
        );
        root.addView(explanation);

        status = new TextView(this);
        root.addView(status);

        Button add_32 = button("+32 MiB");
        add_32.setOnClickListener(view -> allocate_mebibytes(32));
        root.addView(add_32);

        Button fill = button("Fill to 70% of this process heap");
        fill.setOnClickListener(view -> fill_to_fraction(0.70));
        root.addView(fill);

        Button release = button("Release pressure memory");
        release.setOnClickListener(view -> {
            allocations.clear();
            allocated_bytes = 0;
            System.gc();
            update_status("released");
        });
        root.addView(release);

        Button back = button("Return to WebView");
        back.setOnClickListener(view -> finish());
        root.addView(back);

        setContentView(root);
        update_status("ready");
    }

    private Button button(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setAllCaps(false);
        return button;
    }

    private void fill_to_fraction(double fraction) {
        long target = (long) (Runtime.getRuntime().maxMemory() * fraction);
        while (allocated_bytes + (8L << 20) <= target) {
            if (!allocate_mebibytes(8)) {
                return;
            }
        }
        update_status("target reached");
    }

    private boolean allocate_mebibytes(int mebibytes) {
        try {
            byte[] block = new byte[mebibytes * 1024 * 1024];
            for (int index = 0; index < block.length; index += 4096) {
                block[index] = (byte) (index >>> 12);
            }
            allocations.add(block);
            allocated_bytes += block.length;
            update_status("allocated");
            return true;
        } catch (OutOfMemoryError error) {
            update_status("pressure process reached its heap limit");
            return false;
        }
    }

    private void update_status(String note) {
        long allocated_mib = allocated_bytes >> 20;
        long max_mib = Runtime.getRuntime().maxMemory() >> 20;
        status.setText(note + "\nallocated=" + allocated_mib + " MiB\nheap-max=" + max_mib + " MiB");
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
