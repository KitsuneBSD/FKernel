extern "C" int __cxa_atexit(void (*func)(void*), void *arg, void *dso_handle) {
    (void)func;
    (void)arg;
    (void)dso_handle;
    // não faz nada, retorna sucesso
    return 0;
}

extern "C" int atexit(void (*func)(void*)) {
    (void)func;
    return 0;
}