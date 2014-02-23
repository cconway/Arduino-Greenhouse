#ifndef FiniteStateMachine_h
#define FiniteStateMachine_h

typedef void (* FSM_HandlerFn) (uint8_t fromState, uint8_t toState);

// Class Definition
class FiniteStateMachine {
  
  public:
    FiniteStateMachine(uint8_t stateCount, FSM_HandlerFn stateWillChangeHandler, FSM_HandlerFn stateDidChangeHandler);
//    ~FiniteStateMachine();
    void initialize();
    int getCurrentState();
    void setCanTransition(uint8_t fromState, uint8_t toState, boolean canTransition);
    boolean canTransitionToState(uint8_t toState);
    boolean transitionToState(uint8_t toState);
//    void setWillLeaveHandler(int state, FSM_HandlerFn handlerFn);
//    void setDidLeaveHandler(int state, FSM_HandlerFn handlerFn);
//    void setWillEnterHandler(int state, FSM_HandlerFn handlerFn);
//    void setDidEnterHandler(int state, FSM_HandlerFn handlerFn);
    
    
    
  private:
    uint8_t _stateCount;
    uint8_t _currentState;
    boolean **_transitionMap;
    
    FSM_HandlerFn _stateWillChangeHandler;
    FSM_HandlerFn _stateDidChangeHandler;
    
//    FSM_HandlerFn *_willLeaveHandlers;
//    FSM_HandlerFn *_didLeaveHandlers;
//    FSM_HandlerFn *_willEnterHandlers;
//    FSM_HandlerFn *_didEnterHandlers;
};

#endif
