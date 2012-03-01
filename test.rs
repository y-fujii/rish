
fun range $n {
	let $i = 0
	while test $i -lt $n {
		yield $i
		let $i = (expr $i + 1)
	}
}

fun enumerate {
	let $i = 0 
	while fetch $e {
		yield $i $e
		let $i = (expr $i + 1)
	}
}

fun at $n ($list) {
	yield $list | enumerate | while fetch $i $e {
		if test $i -eq $n {
			yield $e
			break 0
		}
	}
}

fun testDefer {
	defer echo
	range 8 | while fetch $i {
		echo -n $i
		defer echo -n $i
	}
	echo
}

fun factorialLoop $n {
	let $result = 1
	range $n | while fetch $i {
		let $result = (expr $result "*" "(" $i + 1 ")" )
	}
	yield $result
}

fun factorialRec $n {
	if test $n -le 1 {
		yield 1
	}
	else {
		expr $n "*" (factorialRec (expr $n - 1))
	}
}

fun ackermann $m $n {
	if test $m -eq 0 {
		expr $n + 1
	}
	else if test $n -eq 0 {
		ackermann (expr $m - 1) 1
	}
	else {
		ackermann (expr $m - 1) (ackermann $m (expr $n - 1))
	}
}

fun qsort0 ($xs) {
	if let $pv ($xs) = $xs {
		qsort0 (yield $xs | while fetch $x {
			if test $x -le $pv {
				yield $x
			}
		})
		yield $pv
		qsort0 (yield $xs | while fetch $x {
			if test $x -gt $pv {
				yield $x
			}
		})
	}
}

fun qsort1 {
	if fetch $pv ($xs) {
		yield $xs | while fetch $x {
			if test $x -le $pv {
				yield $x
			}
		} | qsort1
		yield $pv
		yield $xs | while fetch $x {
			if test $x -gt $pv {
				yield $x
			}
		} | qsort1
	}
}

fun redirect {
	test.rs >| cat | wc
}

fun returnInPipe {
	range 16 | while fetch $i {
		yield $i
		if test $i -eq 8 {
			return 0
		}
	}
	yield "last stmt"
}

fun runTest {
	let $var = S
	echo P(yield a b c)(range 4)$var
	factorialRec 16
	factorialLoop 16
	ackermann 3 2
	testDefer
	at 4 (range 16)
	redirect
	echo (qsort0 0 3 2 6 -10 12312 23 2 98 8)
	echo (yield 0 3 2 6 -10 12312 23 2 98 8 | qsort1)
	returnInPipe
}

runTest
