# ISE Project BSE

Point-of-sale application for the BSE-3A ISE project team.

## Requirements

- CMake 3.21 or newer
- A C++17 compiler
- Qt 6 with `Widgets`, `Multimedia`, and `MultimediaWidgets` for the GUI
- `just` is optional but recommended for the project commands below

## Build and test the core

The core library and tests do not require Qt:

```sh
just core
just test
```

Equivalent CMake commands are:

```sh
cmake -S . -B build/core -DQR_BUILD_APP=OFF -DQR_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --config Release
ctest --test-dir build/core -C Release --output-on-failure
```

The core test suite covers catalog, category, barcode, order, billing, season,
scanner-pipeline, stock, and manager-report behavior.

## Build and run the GUI

Set `QT_PREFIX` when Qt is not already discoverable by CMake. For example:

```sh
export QT_PREFIX=/path/to/Qt/6.x.x/<kit>
just app
just run
```

On Windows, set `QT_PREFIX` to the Qt kit directory in PowerShell before running
`just app`.

The application copies the contents of `data/` beside the built executable so
sample products and barcode images are available at runtime.

## Runtime data

The application uses space-separated text records terminated by `#`:

- `data/Product.txt` — product catalog, prices, tax, discounts, stock, and
  barcode image paths
- `data/PCategory.txt` — product categories
- `data/Seasons.txt` — active seasonal discount ranges
- `Order.txt` — saved orders
- `OrderDetail.txt` — order item quantities
- `Bill.txt` — one persisted bill header per bill
- `BillDetail.txt` — persisted bill item lines

`Order.txt`, `OrderDetail.txt`, `Bill.txt`, and `BillDetail.txt` are resolved
from the working directory. The core services accept explicit paths so tests
can use isolated files.

## Increment 3 core status

Implemented and tested core services currently include:

- Product categories and category-aware product/barcode validation
- Product creation, editing, stock correction, and atomic stock adjustments
- Order persistence, updates, hold/resume status support, and paid-order policy
- Bill and bill-detail persistence with seasonal and special discounts
- Season validation, overlap prevention, and active-percentage lookup
- Manager reports for:
  - incomplete and completed orders
  - cash sales by daily, weekly, monthly, yearly, or custom period
  - all sales and discount totals
  - top sold items
  - top remaining-stock items

`tests/test_reports.cpp` covers the public reports interface, including quote
versus persisted bill totals, calendar grouping, custom ranges, discounts, and
item rankings.

The Qt-free suite currently passes 113/113 tests after restoring billing edge
cases removed during PR #13. The cashier/manager UI and final billing/PDF
integration are still being integrated with Person D's pending UI work.

## Team workflow

Before submitting integration work, run:

```sh
just check
```

This runs the core tests and the Qt application build. Keep runtime data files
out of commits unless a fixture or sample-data change is intentional.
