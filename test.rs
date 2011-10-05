
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
			return 0
		}
	}
	return 1
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

fun runTest {
	let $var = S
	echo P(yield a b c)(range 4)$var
	factorialRec 8
	factorialLoop 8
	testDefer
	at 4 (range 16)
}

runTest
