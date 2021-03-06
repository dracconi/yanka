#define JANET_EV

#include <janet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef JANET_WINDOWS
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <netinet/tcp.h>
#include <fcntl.h>
#endif



typedef struct {
	JanetStream* stream;
	SSL* sec;
	SSL_CTX* ctx;
} SecureConn;


const SSL_METHOD *method;

SSL_CTX *ctx;


static Janet cfun_write(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 2);
	SecureConn *sc = janet_unwrap_abstract(argv[0]);
	int ret = 0;
	if (janet_checktype(argv[1], JANET_BUFFER)) {
		JanetBuffer *buf = janet_getbuffer(argv, 1);
		ret = SSL_write(sc->sec, buf->data, buf->count);
	} else {
		JanetByteView buf = janet_getbytes(argv, 1);
		ret = SSL_write(sc->sec, buf.bytes, buf.len);
	}
	if (ret <= 0) {
		janet_panic("SSL Write failed!");
	}
	return janet_wrap_integer(ret);
}
static Janet cfun_read(int32_t argc, Janet *argv) {
	janet_arity(argc, 2, 4);
	SecureConn *sc = janet_unwrap_abstract(argv[0]);
	JanetBuffer *buf = janet_optbuffer(argv, argc, 2, janet_getinteger(argv, 1));
	if (janet_optinteger(argv, argc, 3, -1) > -1) {
		select(1, &sc->stream->handle, NULL, NULL, janet_getinteger(argv, 3));
	}
	int ret = SSL_read(sc->sec, buf->data, buf->capacity);
	if (ret < 0) {
		janet_panic("SSL Read failed!");
	}
	buf->count = ret;
	return janet_wrap_buffer(buf);
}

const JanetAbstractType secure_socket_type = {
	"ssl/socket",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static Janet cfun_wrap(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 2);
	SecureConn *sc = janet_abstract(&secure_socket_type, sizeof(SecureConn));
	sc->stream = janet_unwrap_abstract(argv[0]); 
	sc->sec = SSL_new(ctx);
	char en = 1;
	setsockopt(sc->stream->handle, IPPROTO_TCP, TCP_NODELAY, &en, 1);
	
	if (SSL_set_tlsext_host_name(sc->sec, janet_getcstring(argv, 1)) != 1) {
		printf("TLSEXT Error");
	}

	if (SSL_set_fd(sc->sec, sc->stream->handle) != 1) {
		janet_panic("Couldn't set file descriptor of SSL");
	}
#ifndef JANET_WINDOWS
	// Disabled non-blocking IO, because it fucks with OpenSSL
	fcntl(sc->stream->handle, F_SETFL, fcntl(sc->stream->handle, F_GETFL) & ~O_NONBLOCK);
#endif

	int ret = SSL_connect(sc->sec);
	if (ret != 1) {
		ERR_print_errors_fp(stderr);
		janet_panicf("SSL Handshake failed %d\n", SSL_get_error(sc->sec, ret));
	}
	en = 0;
	setsockopt(sc->stream->handle, IPPROTO_TCP, TCP_NODELAY, &en, 1);

	return janet_wrap_abstract(sc);
}
static Janet cfun_close(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);
	SecureConn *sc = janet_unwrap_abstract(argv[0]);
	JanetStream *st = sc->stream;
#ifndef JANET_WINDOWS
	fcntl(st->handle, F_SETFL, fcntl(st->handle, F_GETFL) & ~O_NONBLOCK);
#endif
	SSL_free(sc->sec);
	return janet_wrap_abstract(st);
}

static Janet cfun_socket(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);
	SecureConn *sc = janet_unwrap_abstract(argv[0]);
	return janet_wrap_abstract(sc->stream);
}

static Janet cfun_init(int32_t argc, Janet *argv) {
	SSL_library_init();
	SSL_load_error_strings();
	method = TLS_client_method();
	ctx = SSL_CTX_new(method);
	return janet_wrap_nil();
}

static const JanetReg cfuns[] = {
	{"socket", cfun_socket, "(yanka-ssl/socket stream)\n\nGets the underlying socket."},
	{"write", cfun_write, "(yanka-ssl/write stream data)\n\nWrite to the SSL connection"},
	{"read", cfun_read, "(yanka-ssl/read stream nbytes)\n\nRead from the SSL connection"},
	{"init", cfun_init, "(yanka-ssl/init)\n\nInitalize OpenSSL."},
	{"wrap", cfun_wrap, "(yanka-ssl/wrap stream)\n\nChange net connection to SSL connection"},
	{"close", cfun_close, "(yanka-ssl/close stream)\n\nCloses SSL connection and returns the normal stream"},
	{NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
	janet_cfuns(env, "yanka-ssl", cfuns);
}
