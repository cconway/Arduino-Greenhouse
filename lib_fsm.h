#ifndef FiniteStateMachine_h
#define FiniteStateMachine_h

typedef void (* FSM_HandlerFn) (int fromState, int toState);

// Class Definition
class FiniteStateMachine {
  
  public:
    FiniteStateMachine(int stateCount);
    void initialize();
    int getCurrentState();
    void setCanTransition(int fromState, int toState, boolean canTransition);
    boolean canTransitionToState(int toState);
    boolean transitionToState(int toState);
    void setWillLeaveHandler(int state, FSM_HandlerFn handlerFn);
    void setDidLeaveHandler(int state, FSM_HandlerFn handlerFn);
    void setWillEnterHandler(int state, FSM_HandlerFn handlerFn);
    void setDidEnterHandler(int state, FSM_HandlerFn handlerFn);
    
  private:
    int _stateCount;
    int _currentState;
    boolean **_transitionMap;
    FSM_HandlerFn *_willLeaveHandlers;
    FSM_HandlerFn *_didLeaveHandlers;
    FSM_HandlerFn *_willEnterHandlers;
    FSM_HandlerFn *_didEnterHandlers;
};

#endif
