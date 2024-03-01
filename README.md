# wd-migrate

C++ utility to convert [wikidata dumps](https://www.wikidata.org/wiki/Wikidata:Main_Page) into the format expected by [QirK](https://github.com/jlscheerer/kgqa?tab=readme-ov-file)

## Build

```sh
git clone --recursive https://github.com/jlscheerer/wd-migrate.git
g++ --std=c++2a -O3 wd_migrate.cc -lpthread
``
