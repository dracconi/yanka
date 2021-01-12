# Yanka

A simple HTTP client for Janet, written in Janet + C for SSL.

SSL is provided through OpenSSL, but apparently does not work everywhere for some reason yet.

In order to install one must fix link flags in `project.janet` to match their own. These are for Windows with OpenSSL compiled with MSVC dynamic `.lib` files.
