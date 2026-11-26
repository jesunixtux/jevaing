# Project recovery / Recuperación de proyectos

Source-code recovery from a compiled executable is not implemented and cannot recreate the original source reliably. Project and scene saves retain `.bak` files to recover a missing main file after an interrupted replacement; this is not a source archive.

The planned optional recovery package would contain the original project encrypted with authenticated encryption and a digital signature. Private decryption/signing keys must stay outside the executable. Key formats, storage, rotation and a recovery test need design and review before implementation.
