import os

os.system("find .git/objects/ -size 0 -exec rm -f {} \;")
os.system("git fetch origin")
