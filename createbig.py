with open("bigfile.bin", "wb") as f:
    f.seek(1024 * 1024 * 1024 * 10 - 1)
    f.write(b'\0')