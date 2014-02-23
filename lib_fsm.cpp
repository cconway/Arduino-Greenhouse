#include "Arduino.h"
#include "lib_fsm.h"

FiniteStateMachine::FiniteStateMachine(uint8_t stateCount, FSM_HandlerFn stateWillChangeHandler, FSM_HandlerFn stateDidChangeHandler) {
  
  _stateCount = stateCount;
  
  _stateWillChangeHandler = stateWillChangeHandler;
  _stateDidChangeHandler = stateDidChangeHandler;
  
  // Malloc space for the boolean lookup table
  _transitionMap = (boolean **) malloc(stateCount * sizeof(boolean *));
  for (int i=0; i < stateCount; i++) {
    _transitionMap[i] = (boolean *) malloc(stateCount * sizeof(boolean));
  }
}

void FiniteStateMachine::initialize() {
  
//  _didEnterHandlers[_currentState](_currentState, _currentState);
  _stateWillChangeHandler(_currentState, _currentState);
  _stateDidChangeHandler(_currentState, _currentState);
}

int FiniteStateMachine::getCurrentState() {
  
  return _currentState;
}

void FiniteStateMachine::setCanTransition(uint8_t fromState, uint8_t toState, boolean canTransition) {
  
  if (fromState < _stateCount && toState < _stateCount) {
    
    _transitionMap[fromState][toState] = canTransition;
    
  }
}

boolean FiniteStateMachine::canTransitionToState(uint8_t toState) {
  
  if (toState < _stateCount) {
    
    return _transitionMap[_currentState][toState];
  
  } else return false;
}

boolean FiniteStateMachine::transitionToState(uint8_t toState) {
  
  if (this->canTransitionToState(toState)) {
    
    if (toState == _currentState) return true;
    else {
    
       int originalState = _currentState;
       
       _stateWillChangeHandler(originalState, toState);
      
      // Run pre-state transition handlers
//      if (_willLeaveHandlers[_currentState]) _willLeaveHandlers[_currentState](_currentState, toState);
//      if (_willEnterHandlers[_currentState]) _willEnterHandlers[_currentState](_currentState, toState);
      
      // Change state
      _currentState = toState;
      
      // Run post-state transition handlers
//      if (_didLeaveHandlers[_currentState]) _didLeaveHandlers[_currentState](_currentState, toState);
//      if (_didEnterHandlers[_currentState]) _didEnterHandlers[_currentState](_currentState, toState);
      
      _stateDidChangeHandler(originalState, toState);
    
      return true;
      
    }
    
  } else return false;
}
