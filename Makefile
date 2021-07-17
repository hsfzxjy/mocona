dev:
	python3 setup.py build_ext --inplace -DTRACE_REFCNT
test:
	PYTHONPATH=. pytest tests/
