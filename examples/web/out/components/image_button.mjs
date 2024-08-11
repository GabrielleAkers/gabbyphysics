export const ImageButton = (src, w, h, on_click, tooltip) => {
    const div = document.createElement("div");
    div.style.margin = "0";
    div.style.padding = "0";
    const img = new Image(w, h);
    img.src = src;
    img.onload = () => {
        div.style.width = w.toString();
        div.style.height = h.toString();
        div.appendChild(img);
    };
    const tt = document.createElement("div");
    tt.id = `img_btn_tooltip-${src}`;
    tt.popover = "manual";
    tt.innerText = tooltip;
    tt.style.inset = "unset";
    div.appendChild(tt);
    img.addEventListener("mouseenter", () => {
        if (!tt.matches(":popover-open"))
            tt.showPopover();
    });
    img.addEventListener("mouseleave", () => {
        if (tt.matches(":popover-open"))
            tt.hidePopover();
    });
    img.addEventListener("mousedown", () => {
        img.style.border = "2px darkgrey solid";
        img.style.filter = "hue-rotate(20deg)";
    });
    img.addEventListener("mouseup", () => {
        img.style.border = "";
        img.style.filter = "";
    });
    img.addEventListener("click", on_click);
    return div;
};
