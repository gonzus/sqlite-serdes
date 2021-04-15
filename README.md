# An example of SQLite's database (de)serialization

This needs an SQLite library compiled with (at least):
```sh
./configure CFLAGS="-DSQLITE_ENABLE_DESERIALIZE=1"
```

With that, simply run `make` and try the executable by running `serdes`.
