# Parallel Downloader

Shrey Shah

Downloads a list of URLs in parallel by running `curl` in child processes, with a cap on how many run at once. See the [top-level README](../README.md#parallel-downloader) for details.

```bash
make
./a4download test_urls.txt 3     # at most 3 downloads at a time
```

Each line of the input file is `<output-name> <url> [timeout-seconds]`.
