unsigned char cont[256];

unsigned char *GetErrorHistory() {
    memset(cont, 0, 256);
    memcpy (cont, error_handler_history -> chr, error_handler_history -> len);
    return cont;
}

uint32_t GetErrorGotoLine() {
    return error_goto_line;
}

