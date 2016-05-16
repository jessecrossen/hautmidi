#include "model.h"

Model::Model() {
  _views = NULL;
  onStorageChanged = NULL;
}

void Model::invalidate() {
  View *view = _views;
  while (view) {
    view->invalidate();
    view = view->_nextModelView;
  }
}

void Model::addView(View *view) {
  if (_views == NULL) {
    _views = view;
    _views->_nextModelView = NULL;
  }
  else {
    View *node = _views;
    while (node->_nextModelView != NULL) node = node->_nextModelView;
    node->_nextModelView = view;
    view->_nextModelView = NULL;
  }
  view->invalidate();
}
void Model::removeView(View *view) {
  View *prev = NULL;
  View *node = _views;
  while (node != NULL) {
    if (node == view) {
      // handle the first child being removed
      if (prev == NULL) _views = node->_nextModelView;
      // handle later children being removed
      else prev->_nextModelView = node->_nextModelView;
    }
    prev = node;
    node = node->_nextModelView;
  }
}

uint8_t Model::storageBytes() { return(0); }
void Model::store(uint8_t *buffer) { }
void Model::recall(uint8_t *buffer) { }

