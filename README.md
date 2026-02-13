# icat (an *i*mproved alternative to *cat*)

icat is an implementation of the popular UNIX command cat made by Torbjörn Granlund, and Richard Stallman.
to understand how the command works, i took a peek at the GNU [coreutils](https://github.com/coreutils/coreutils/blob/master/src/cat.c) library

### Why is icat an "improved" alternative to cat?
icat can parse through structured data like JSON, XML and CSV. (IMPORTANT: due to time constraint, it has not been implemented yet, but, it is on the agenda).

# Build
```bash
git clone https://github.com/dashfetch/icat.git
cd icat
make

./icat -h
```

# Add icat to PATH in Linux (optional)
``` bash
# Use your preferred editor

vim ~/.zshrc or vim ~/.bashrc
export PATH ="PATH:/path/to/icat/folder/"
```

##  Author
[@dashfetch](x.com/dashfetch)
