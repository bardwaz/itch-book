# ITCH 5.0 Sample Data

## Download

Nasdaq provides historical ITCH 5.0 data files via FTP:

```
ftp://emi.nasdaq.com/ITCH/
```

Files are named: `MMDDYYYY.NASDAQ_ITCH50.gz`

### Quick Download (one trading day)

```bash
# Download a single day (Jan 30, 2019 — ~3 GB compressed)
wget ftp://emi.nasdaq.com/ITCH/01302019.NASDAQ_ITCH50.gz

# Decompress (~5-8 GB uncompressed)
gunzip 01302019.NASDAQ_ITCH50.gz
```

### Expected File Sizes

| | Compressed (.gz) | Uncompressed |
|---|---|---|
| One trading day | 2–5 GB | 5–12 GB |

## Usage

Place the uncompressed `.NASDAQ_ITCH50` file in this directory, then run:

```bash
../build-release/src/itch-book 01302019.NASDAQ_ITCH50 --stats
```

> **Note:** These files are not tracked by git (see `.gitignore`).
