dev:
	python3 setup.py build_ext -b . -DTRACE_REFCNT
test:
	PYTHONPATH=. pytest tests/
