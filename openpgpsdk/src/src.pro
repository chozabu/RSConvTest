TEMPLATE = lib
CONFIG = staticlib debug

DEFINES *= OPENSSL_NO_IDEA 

QMAKE_CXXFLAGS *= -Wall -Werror -W 

TARGET = ops
DESTDIR = ../lib
DEPENDPATH += .
INCLUDEPATH += . ../include

# Input
HEADERS += keyring_local.h parse_local.h
SOURCES += accumulate.c \
           compress.c \
           create.c \
           crypto.c \
           errors.c \
           fingerprint.c \
           hash.c \
           keyring.c \
           lists.c \
           memory.c \
           openssl_crypto.c \
           packet-parse.c \
           packet-print.c \
           packet-show.c \
           random.c \
           reader.c \
           reader_armoured.c \
           reader_encrypted_se.c \
           reader_encrypted_seip.c \
           reader_fd.c \
           reader_hashed.c \
           reader_mem.c \
           readerwriter.c \
           signature.c \
           symmetric.c \
           util.c \
           validate.c \
           writer.c \
           writer_armour.c \
           writer_encrypt.c \
           writer_encrypt_se_ip.c \
           writer_fd.c \
           writer_memory.c \
           writer_skey_checksum.c \
           writer_stream_encrypt_se_ip.c
