#ifndef _HOODWIND_model_h_
#define _HOODWIND_model_h_

#include "view.h"
class View;

class Model {
  public:
    Model();
    
    // mark all views as needing an update when the model changes
    void invalidate();
    // add/remove a view representing the model
    void addView(View *view);
    void removeView(View *view);
    
    // methods to persist the model's important data
    virtual uint8_t storageBytes();
    virtual void store(uint8_t *buffer);
    virtual void recall(uint8_t *buffer);
    void (*onStorageChanged)();
    
  private:
    // the root of a linked list of views representing this model
    View *_views;
};

#endif
