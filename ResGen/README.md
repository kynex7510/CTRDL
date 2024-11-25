# ResGen

Resolver generator.

```
cmake -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --config Release
```

```
$DEVKITARM/bin/arm-none-eabi-readelf -W -s my.so | grep UND | awk '{ print $8 }' > out.symlist
```