dev:
	python3 setup.py build_ext -b . -DTRACE_REFCNT
clean:
	rm mocona/*.so
install:
	python3 setup.py install --user
test:
	PYTHONPATH=. pytest tests/
