typedef void (*coroutine_run_t)(void *arg);

void coroutine_exit(void) {
	std::cout<<"Coroutine has exited.  Goodbye.\n";
	exit(0);
}

class coroutine {
public:
	long *stack; // top of coroutine's stack
	long *stack_alloc; // allocated memory for stack
	
	// Used to make main into a coroutine
	coroutine() {
		stack=0;
		stack_alloc=0;
	}
	// Used to create a new coroutine
	coroutine(coroutine_run_t run,void *arg,int stacksize=1024*1024) {
		stack_alloc=new long[stacksize/sizeof(long)];
		stack=&stack_alloc[stacksize/sizeof(long)-1]; // top of stack
		*(--stack)=(long)coroutine_exit; // coroutine cleanup 
		*(--stack)=(long)run; // user's function to run (rop style!)
		*(--stack)=(long)arg; // user's function argument (rdi)
		for (int saved=0;saved<6;saved++)
			*(--stack)=0xdeadbeef; // initial values for saved registers
	}
	// Cleanup
	~coroutine() {
		delete[] stack_alloc;
	}
};

extern "C" void swap64(coroutine *old_co,coroutine *new_co);

coroutine *main_co;
coroutine *sub_co;

void sub(void *arg) {
	int i; // random local, to see stack pointer
	std::cout<<"  Inside sub-coroutine.  Stack="<<&i<<endl;
	swap64(sub_co,main_co); // back to main
	std::cout<<"  Back in sub.  Stack="<<&i<<endl;
	swap64(sub_co,main_co); // back to main
}

long foo(void) {
	int i;
	std::cout<<"Making coroutines.  Stack="<<&i<<std::endl;
	main_co=new coroutine();
	sub_co=new coroutine(sub,0);
	std::cout<<"Switching to coroutine"<<std::endl;
	swap64(main_co,sub_co);
	std::cout<<"Back in main from coroutine.  Stack="<<&i<<std::endl;
	swap64(main_co,sub_co);
	std::cout<<"Any questions?"<<std::endl;
	return 0;
}