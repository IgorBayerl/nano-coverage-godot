import shutil
import os

src = "demo/bin"
dst = "build_demo/bin"

if not os.path.exists(dst):
    os.makedirs(dst)

for f in os.listdir(src):
    if f.endswith(".dll") or f.endswith(".gdextension"):
        print(f"Copying {f}...")
        shutil.copy2(os.path.join(src, f), os.path.join(dst, f))

print("Copy complete.")
