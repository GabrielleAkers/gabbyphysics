export const SevenSegmentDisplay = (num_digits, digit_width, digit_height, initial_val, label = "", digit_color = "white") => {
    const display = document.createElement("div");
    display.style.display = "flex";
    display.style.margin = `${digit_width / 10}px`;
    display.style.padding = "2px";
    display.style.position = "relative";
    let backing_val = initial_val;
    const displays = [];
    for (let i = 0; i < num_digits; i++) {
        const canv = document.createElement("canvas");
        canv.width = digit_width;
        canv.height = digit_height;
        const ctx = canv.getContext("2d");
        if (!ctx)
            throw new Error("2d canvas context not supported");
        displays.push([canv, ctx]);
        display.appendChild(canv);
    }
    if (label) {
        const lbl = document.createElement("p");
        const lbl_holder = document.createElement("div");
        lbl_holder.style.width = "6px";
        display.appendChild(lbl_holder);
        lbl.style.color = "white";
        lbl.innerText = label;
        lbl.style.margin = "2px";
        lbl.style.position = "absolute";
        lbl.style.bottom = "0";
        lbl.style.right = "-2fr";
        lbl_holder.appendChild(lbl);
    }
    const p_w = digit_width / 10;
    const p_h = digit_height / 10;
    /** x->
     * - A -  y
     * F   B  |
     * - G - \/
     * E   C
     * - D -
     */
    const segments = {
        // [start, end]
        A: [[2 * p_w, p_h], [digit_width - 2 * p_w, p_h]],
        B: [[digit_width - p_w, 2 * p_h], [digit_width - p_w, digit_height / 2 - p_h / 2]],
        C: [[digit_width - p_w, digit_height / 2 + p_h / 2], [digit_width - p_w, digit_height - 2 * p_h]],
        D: [[2 * p_w, digit_height - p_h], [digit_width - 2 * p_w, digit_height - p_h]],
        E: [[p_w, digit_height - 2 * p_h], [p_w, digit_height / 2 + p_h / 2]],
        F: [[p_w, digit_height / 2 - p_h / 2], [p_w, 2 * p_h]],
        G: [[3 * p_w, digit_height / 2], [digit_width - 3 * p_w, digit_height / 2]]
    };
    const draw_segment = (ct, seg, color = "white") => {
        if (!Object.hasOwn(segments, seg))
            return;
        ct.beginPath();
        const segment = segments[seg];
        ct.moveTo(segment[0][0], segment[0][1]);
        ct.lineTo(segment[1][0], segment[1][1]);
        ct.strokeStyle = color;
        ct.lineWidth = 4;
        ct.stroke();
        ct.closePath();
    };
    const draw_segments = (ct, segs, color = "white") => {
        ct.clearRect(0, 0, ct.canvas.width, ct.canvas.height);
        segs.forEach(s => {
            draw_segment(ct, s, color);
        });
    };
    const zero_display = (color = "rgba(255, 255, 255, 0.1)") => {
        for (let i = 0; i < num_digits; i++) {
            draw_segments(displays[i][1], ["A", "B", "C", "D", "E", "F"], color);
        }
    };
    const display_num_in_display = (ct, n, color = "white") => {
        if (n >= 10 || n < 0)
            throw new Error("n must be 0 <= n <= 9");
        let segs = [];
        let c = color;
        switch (n) {
            case 0:
                segs = ["A", "B", "C", "D", "E", "F"];
                c = "rgba(255, 255, 255, 0.1)";
                break;
            case 1:
                segs = ["B", "C"];
                break;
            case 2:
                segs = ["A", "B", "D", "E", "G"];
                break;
            case 3:
                segs = ["A", "B", "C", "D", "G"];
                break;
            case 4:
                segs = ["B", "C", "F", "G"];
                break;
            case 5:
                segs = ["A", "C", "D", "F", "G"];
                break;
            case 6:
                segs = ["A", "C", "D", "E", "F", "G"];
                break;
            case 7:
                segs = ["A", "B", "C"];
                break;
            case 8:
                segs = ["A", "B", "C", "D", "E", "F", "G"];
                break;
            case 9:
                segs = ["A", "B", "C", "F", "G"];
                break;
        }
        draw_segments(ct, segs, c);
    };
    const pad_n = (from, to, pad_with) => {
        for (let i = from; i < to; i++) {
            display_num_in_display(displays[i][1], pad_with);
        }
    };
    const display_num = (n, color = "white") => {
        zero_display();
        const int_n = parseInt(n.toString());
        let str_n = int_n.toString();
        let len_n = str_n.length;
        if (len_n > num_digits)
            throw new Error(`Not enough digits to display ${n}`);
        let diff = num_digits - len_n;
        // pad_n(0, diff, 0);
        for (let i = 0; i < len_n; i++) {
            display_num_in_display(displays[diff + i][1], parseInt(str_n[i]), color);
        }
    };
    const set_val = (n) => {
        backing_val = n;
        display_num(backing_val);
    };
    display_num(backing_val, digit_color);
    return [display, set_val];
};
