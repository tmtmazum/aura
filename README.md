# Fetching dependencies

aura's has the following submodule dependencies

- Cinder
- Cinder-Boost (subset of boost used by cinder)

Cinder will need to be built into a static library consumed
by the other projects. Cinder-boost however is only a header-only
library used by Cinder itself.

Run the following command to update all submodules

```
git submodule update --init --recursive
git submodule update --remote
```
