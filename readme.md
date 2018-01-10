# rish - Modern Unix shell language
[![Build Status](https://travis-ci.org/y-fujii/rish.svg?branch=master)](https://travis-ci.org/y-fujii/rish)

## Example
```
fun enumerate {
	let $i = 0 
	while fetch $e {
		yield $i $e
		let $i = $i + 1
	}
}

fun even_elements {
	enumerate | while fetch $i $e {
		if ($i % 2 == 0) {
			yield $e
		}
	}
}

yield a b c d | even_elements | sort -r
```
The result is:
```
d
b
```

## Current status

The development is currently stalled.
