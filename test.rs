
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

fun redirect {
	test.rs >| cat | wc
}

fun runTest {
	let $var = S
	echo P(yield a b c)(range 4)$var
	factorialRec 16
	factorialLoop 16
	ackermann 3 3
	testDefer
	at 4 (range 16)
	redirect
}

runTest
