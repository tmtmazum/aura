# Building Aura
## Fetching Build Dependencies

aura's has the following submodule dependencies

- Cinder
- Boost (filesystem for Cinder, System, Beast for server/client websockets)

Run the following command to update all submodules

```
git submodule update --init --recursive
git submodule update --remote
```

In order to pre-build boost on Windows (using boost-build instead of cmake):
```
cd external/boost
git submodule update --init --recursive
.\bootstrap.bat
b2 runtime-link=static address-model=64 debug-symbols=off optimizations=speed --with-filesystem --with-system --with-date_time
```
