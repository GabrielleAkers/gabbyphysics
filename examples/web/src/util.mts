export function cstrlen(buff: Uint8Array, ptr: number) {
    let len = 0;
    while (buff[ptr] !== 0) {
        len++;
        ptr++;
    }
    return len;
}

export function cstr_by_ptr(buff: ArrayBuffer, ptr: number) {
    const mem = new Uint8Array(buff);
    const len = cstrlen(mem, ptr);
    const bytes = new Uint8Array(buff, ptr, len);
    return new TextDecoder().decode(bytes);
}
