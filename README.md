# Alex C Library

## Collections

Not thread safe.

### SList

- Containerless singly linked list.
- NULL values permitted.

### PSet

- Array backed pointer set.
- Entries preserve insertion order.
- Operations linearly traverse values.
- NULL not permitted.

### SSet

- `PSet` with string values

### PTable

- Array backed pointer indexed table.
- Entries preserve insertion order.
- Operations linearly traverse keys.
- NULL values permitted.

### ITable

- `PTable` with `size_t` keys

### STable

- `PTable` with string keys.

## Strings

libc string helpers 
