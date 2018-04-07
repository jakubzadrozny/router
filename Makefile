CC = c++
CFLAGS = -std=c++11 -Wall -Wextra

BIN = router
OBJS = utils.o net_utils.o handler.o

$(BIN): $(OBJS) $(BIN).cpp
	$(CC) $(CFLAGS) $(OBJS) $(BIN).cpp -o $(BIN)

%.o: %.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o

distclean:
	rm -f *.o $(BIN)
