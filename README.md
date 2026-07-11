# Alex C Library

## Collections

Not thread safe.

### Pslist

* Containerless singly linked list.
* NULL values permitted.

### PSet

* Array backed pointer set.
* Entries preserve insertion order.
* Operations linearly traverse values.
* NULL not permitted.

### SSet

* `PSet` with string values
* Values are memory managed.

### PPmap

* Array backed pointer indexed map.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.

### IPmap

* `PPmap` with `size_t` keys

### SMap

* `PPmap` with string keys.
* Keys are memory managed.

### SSmap

* `PPmap` with string keys and vals.
* Keys and values are memory managed.

## Strings

libc string helpers 
