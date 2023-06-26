# Alex C Collections

## OSet

* Array backed ordered set.
* Operations linearly traverse values.
* Non NULL values only.

## ITable

* Array backed integer indexed table.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values not permitted.

## PTable

* Array backed pointer indexed table.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values not permitted.
* Convenience wrapper around ITable.
