# NOSKV
a vc++ project for cross process key/value base on CPPLMDB

in windows, we need some solutions to  access cross process k/v,  generally, we can use ini file, but ini file does not support big size and complex structure.

NOSKV base on CPPLMDB, it can access kv cross process, and also accept any value change by subscript special key.
base on lmdb, you do not need to worry about the LOCK from different process or thread.

