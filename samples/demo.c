/* samples/demo.c - Embedded telemetry processing system demonstrating all analyzer rules */

int g_channel = 1;

/* Triggers W004 (missing return) and W011 (unsafe function) */
int process_packet_header(char *dest, char *src, int flag) {
    strcpy(dest, src);
    if (flag > 0) {
        return 1;
    }
}

/* Long telemetry pipeline: triggers W008 (> 40 lines), W007 (nesting > 3),
   W001 (unused variable), W002 (assignment in condition), W003 (unreachable code),
   W005 (use before assignment), W006 (magic number), W009 (empty body),
   W012 (shadowing), and W013 (division by zero) */
int run_telemetry_pipeline(int g_channel, int raw_input) {
    int unused_metric;
    int mode = 0;
    int uninit_bias;
    int reading = uninit_bias + 1;
    int magic_scale;
    magic_scale = 42;

    if (mode = raw_input) {
        reading += 1;
    }

    if (reading > 0);

    if (g_channel > 0) {
        while (raw_input > 0) {
            for (int i = 0; i < 2; i++) {
                if (reading > 1) {
                    raw_input--;
                }
            }
        }
    }

    int div_zero_test = reading / 0;

    reading += magic_scale + div_zero_test;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    return reading;

    reading = 0;
}

/* Evaluates subsystem status: triggers W010 (switch without default) and W014 (fall-through case) */
int evaluate_subsystem(int command) {
    int status = 0;
    switch (command) {
        case 1:
            status = 1;
        case 2:
            status = 2;
            break;
    }
    return status;
}

int main(void) {
    char *buf = "packet_buf";
    char *src = "telemetry_src";
    int header = process_packet_header(buf, src, 1);
    int telemetry = run_telemetry_pipeline(header, 1);
    int sub = evaluate_subsystem(telemetry);
    return sub;
}
