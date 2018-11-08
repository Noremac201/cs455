import argparse
import os
import sys

from cryptography.exceptions import InvalidTag
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC

SALT_LEN = 16
TAG_LEN = 16
IV_LEN = 12


def aes_decrypt_file(f_name, key, mode=modes.GCM):
    """
    Decrypts a given file's contents from the given key.

    It reads the salt, IV, and GCM tag from the file, and then attempts to decipher the
    cipher text

    :param f_name: File who's contents will be decrypted
    :param key: Key to decrypt the contents with
    :param mode: AES mode, defaults to GCM
    :return: The original plain text if decryption is correct.
    """

    with open(f_name, 'rb') as f:
        text = f.read()

        salt = text[0:SALT_LEN]
        iv = text[SALT_LEN:SALT_LEN + IV_LEN]
        tag = text[SALT_LEN + IV_LEN:SALT_LEN + TAG_LEN + IV_LEN]
        cipher_text = text[SALT_LEN + IV_LEN + TAG_LEN:]

        key = derive_key(key.encode('utf-8'), salt)
        cipher = Cipher(algorithm=algorithms.AES(key),
                        mode=mode(iv),
                        backend=default_backend())
        decryptor = cipher.decryptor()

        try:
            return decryptor.update(cipher_text) + decryptor.finalize_with_tag(tag)
        except InvalidTag:
            raise Exception("Unable to decrypt text.")


def aes_encrypt_file(f_name, key, iv=os.urandom(IV_LEN), mode=modes.GCM):
    """
    Encrypts the contents of a file, and saves it to "f_name.aes"

    The file is laid out with the first 16 bytes as salt, next 12 as IV, and next 16 as GCM tag,
    with the remaining bytes the cipher text to decrypt.

    :param f_name: File to read and encrypt
    :param key: The key to encrypt the file with
    :param iv: The initialization vector, if not supplied, is a 12 byte random number,
        12 bytes has been shown to be the best if its random, since it doesn't require
        additional computations to encrypt it, but is still computationally secure.
    :param mode: The encryption mode, defaults to GCM, the method only uses AES to create
        cipher text
    """
    salt = os.urandom(SALT_LEN)
    key = derive_key(key.encode('utf-8'), salt)

    encryptor = Cipher(algorithm=algorithms.AES(key),
                       mode=mode(iv),
                       backend=default_backend()).encryptor()

    with open(f_name, 'rb') as f:
        f_text = f.read()
        cipher_text = encryptor.update(f_text) + encryptor.finalize()
        with open(f_name + '.aes', 'wb') as o:
            # salt 16 bytes
            # iv 12 bytes
            # tag 16 bytes
            o.write(salt)
            o.write(iv)
            o.write(encryptor.tag)
            o.write(cipher_text)


def derive_key(key, salt):
    """
    Given a key and a salt, derives a cryptographically secure key to be used in
    following computations. This is to allow any size key as input to the program, as
    we can extend it to the required multiple of 16,24,32 that AES requires

    :return:  Derived key from python's cryptography library
    """
    backend = default_backend()
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=2 ** 20,
        backend=backend
    )
    return kdf.derive(key)


def main():
    args = parse_args(sys.argv[1:])

    if args.encrypt:
        aes_encrypt_file(f_name=args.input,
                         key=args.key)
    else:
        aes_decrypt_file(f_name=args.input,
                         key=args.key)


def parse_args(args):
    parser = argparse.ArgumentParser(description='Encrypt a file with AES encryption.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--encrypt', help='Flag that we encrypt the file.', action='store_true')
    group.add_argument('-d', '--decrypt', help='Flag decrypt the file.', action='store_true')
    parser.add_argument('-k', '--key', help='The key to encrypt the file with', required=True)
    parser.add_argument('-i', '--input', help='The data file you want hidden', required=True)
    return parser.parse_args(args)


if __name__ == '__main__':
    main()
