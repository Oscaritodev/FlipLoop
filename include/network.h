#pragma once
#include <3ds.h>

Result network_init();
void network_exit();

// Realiza una petición POST al servidor.
// json_body: El cuerpo JSON a enviar como string.
// out_buffer: Donde guardar la respuesta.
Result network_post(const char* endpoint, const char* json_body, char* out_buffer, size_t buffer_size);

// Realiza una petición GET al servidor.
// token: Token de sesión opcional.
Result network_get(const char* endpoint, const char* token, char* out_buffer, size_t buffer_size);
