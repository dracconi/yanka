#define JANET_EV

#include <janet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef JANET_WINDOWS
#include <winsock.h>
#else
#include <sys/socket.h>
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
	SecureConn *sc = janet_getpointer(argv, 0);
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
	janet_fixarity(argc, 2);
	SecureConn *sc = janet_getpointer(argv, 0);
	JanetBuffer *buf = janet_buffer(janet_getinteger(argv, 1));
	int ret = SSL_read(sc->sec, buf->data, buf->capacity);
	if (ret < 0) {
		janet_panic("SSL Read failed!");
	}
	buf->count = ret;
	return janet_wrap_buffer(buf);
}
static Janet cfun_wrap(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);
	SecureConn *sc = janet_smalloc(sizeof(SecureConn));
	sc->stream = janet_unwrap_abstract(argv[0]); 
	sc->sec = SSL_new(ctx);

	if (SSL_set_fd(sc->sec, sc->stream->handle) != 1) {
		janet_panic("Couldn't set file descriptor of SSL");
	}

	char en = 1;
	setsockopt(sc->stream->handle, IPPROTO_TCP, TCP_NODELAY, &en, 1);
	int ret = SSL_connect(sc->sec);
	if (ret != 1) {
		ERR_print_errors_fp(stderr);
		janet_panicf("SSL Handshake failed\n");
	}

	return janet_wrap_pointer(sc);
}
static Janet cfun_close(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);
	SecureConn *sc = janet_getpointer(argv, 0);
	JanetStream *st = sc->stream;
	SSL_free(sc->sec);
	janet_sfree(sc);
	return janet_wrap_abstract(st);
}

static Janet cfun_socket(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);
	SecureConn *sc = janet_getpointer(argv, 0);
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
	{"socket", cfun_socket, "(ssl/socket stream)\n\nGets the underlying socket."},
	{"write", cfun_write, "(ssl/write stream data)\n\nWrite to the SSL connection"},
	{"read", cfun_read, "(ssl/read stream nbytes)\n\nRead from the SSL connection"},
	{"init", cfun_init, "(ssl/init)\n\nInitalize OpenSSL."},
	{"wrap", cfun_wrap, "(ssl/wrap stream)\n\nChange net connection to SSL connection"},
	{"close", cfun_close, "(ssl/close stream)\n\nCloses SSL connection and returns the normal stream"},
	{NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
	janet_cfuns(env, "ssl", cfuns);
}
