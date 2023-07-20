CC = icpc
CFLAGS =
COPTFLAGS = -O3 -g -qopenmp
LDFLAGS =

default:
	@echo "=================================================="
	@echo "To build your sorting code, use:"
	@echo "  make mergesort-omp    	 # For Mergesort"
	@echo ""
	@echo "To clean this subdirectory (remove object files"
	@echo "and other junk), use:"
	@echo "  make clean"
	@echo "=================================================="

# Mergesort driver
mergesort-omp: driver.o sort.o mergesort-omp.o
	$(CC) $(COPTFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $(COPTFLAGS) -o $@ -c $<

clean-omp:
	rm -f core *.o *~ mergesort-omp

# mergesort-easy driver
mergesort-easy: driver.o sort.o mergesort-easy.o
	$(CC) $(COPTFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $(COPTFLAGS) -o $@ -c $<

clean-easy:
	rm -f core *.o *~ mergesort-easy

# eof
