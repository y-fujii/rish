fun size {
	let $n = 0
	while fetch $e {
		let $n = (expr $n + 1)
	}
	yield $n
}

fun index $n {
	enumerate | while fetch $i $e {
		if test $i -eq $n {
			yield $e
			break 0
		}
	}
}

fun slice $bgn $end {
	enumerate | while fetch $i $e {
		if test $bgn -le $i && test $i -lt $end {
			yield $e
		}
	}
}

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
		qsort0 $xs( while fetch $x {
			if test $x -le $pv {
				yield $x
			}
		} )
		yield $pv
		qsort0 $xs( yield $xs | while fetch $x {
			if test $x -gt $pv {
				yield $x
			}
		} )
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

fun nested {
	let $p = "OK"
	let $q = "BAD"
	let $r = "OK"
	fun inner $r {
		let $q = "OK"
		yield $p
		yield $q
	}
	inner "BAD"
	yield $r
}

fun runBg {
	let $t = (& { sleep 2 ; yield "finished thread" })
	sys.join $t
	yield "finished join"
}

fun testSlice {
	let ($arr) = (range 8)
	echo $arr(0)
	echo $arr(0 2)
	echo $arr(0 -1)
	echo $arr(-1 0)

	let ($err) =
	! echo $err(0) &&
	! echo $err(0 2) &&
	! echo $err(0 -1) &&
	! echo $err(-2 -1) &&
	echo "slice OK"
}

fun emulPrevNext {
	let $cwDir = "/0"

	fun pwd {
		yield $cwDir
	}

	fun std.cd $dir {
		let $cwDir = $dir
		echo $dir
	}

	let ($dirStack) = (pwd)
	let $dirPos = 0

	fun cd $dir {
		if std.cd $dir {
			let ($dirStack) = $dirStack(slice 0 (expr $dirPos + 1)) (pwd)
			let $dirPos = (expr $dirPos + 1)
		}
	}

	fun nd {
		if expr $dirPos + 1 < $dirStack(size) |> /dev/null {
			let $dirPos = (expr $dirPos + 1)
			std.cd $dirStack(index $dirPos)
		}
	}

	fun pd {
		if expr $dirPos - 1 >= 0 |> /dev/null {
			let $dirPos = (expr $dirPos - 1)
			std.cd $dirStack(index $dirPos)
		}
	}

	echo (
		cd /1
		cd /2
		cd /3
		pd
		cd /4
		pd
		pd
		nd
		range 16 | while fetch $i {
			pd
		}
	)
	echo /1 /2 /3 /2 /4 /2 /1 /2 /1 /0
}

fun runTest {
	let $var = S
	echo P(yield a b c)(range 4)$var
	factorialRec 16
	factorialLoop 16
	ackermann 3 2
	echo 29
	# testSlice
	testDefer
	range 16 | index 4
	range 16 | slice 4 6
	redirect
	echo (qsort0 0 3 2 6 -10 12312 23 2 98 8)
	echo (yield 0 3 2 6 -10 12312 23 2 98 8 | qsort1)
	returnInPipe
	nested
	runBg # comment
	emulPrevNext
	#comment

	if true { # comment
		true
	} # comment
	else {
		#comment #comment
	}
}

runTest
