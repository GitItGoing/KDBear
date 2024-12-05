
# KDBear

KDBear is an open-source C++ library designed to simplify interactions with KDB+, a high-performance time-series database. Inspired by the usability of Python's Pandas, KDBear offers an intuitive and modern API for performing complex data manipulations, enabling developers to harness the power of KDB+ efficiently.

---

## Features

### Data Handling Functions
- **`read_csv`**: Imports data from CSV files into KDB+.
- **`make_table`**: Creates tables using 2D vectors to KDB+ tables for data manipulation.
- **`iloc`**: Selects rows and columns by integer location, similar to Pandas.
- **`loc`**: Selects rows and columns based on labels or conditions.
- **`get_metadata`**: Retrieves metadata from tables in KDB+.
- **`shape`**: Returns the dimensions of a table in rows and columns.
- **`print_result`**: Outputs the results of a query in a readable format - general purpose printing.
- **`print_head`**: Displays the first few rows of a table for quick inspection.
- **`print_tail`**: Displays the last few rows of a table for quick inspection.

### Join Operations
- **`inner_join`**: Combines tables by matching keys, keeping only matching rows.
- **`left_join`**: Combines tables by matching keys, keeping all rows from the left table.
- **`right_join`**: Combines tables by matching keys, keeping all rows from the right table.
- **`asof_join`**: Joins tables based on time or sorted keys, keeping the nearest match.
- **`window_join`**: Combines data within a specified time or range window.
- **`union_join`**: Combines two tables by appending rows.

### Utility Functions
- **`k_to_vector`**: Converts KDB+ data types to C++ vectors for further processing.

### Connection Management
- **`connect`**: Establishes a connection to a KDB+ instance.
- **`disconnect`**: Terminates the connection to a KDB+ instance.

---

## How to Build and Run the Demo

1. Clone the repository:
   ```bash
   git clone https://github.com/your_username/kdbear.git
   cd kdbear
   ```

2. Build the project:
   ```bash
   make
   ```

3. Start your KDB+ Server (example uses port 6000)
   ```
   q -p 6000
   ```
   
4. Run the demo with example CSV files:
   ```bash
   ./kdbear_demo demo/quotes_example.csv demo/trades_example.csv
   ```

---

## Documentation

For detailed instructions, examples, and API references, please visit the [KDBear Documentation](https://www.kdbear.net/documentation).

---

## Contact

If you have any questions about usage, suggestions, or run into any issues, feel free to contact me at:
**nathan DOT ginis AT gmail DOT com**

---

### License

This project is licensed under the MIT License. See the `LICENSE` file for details.
