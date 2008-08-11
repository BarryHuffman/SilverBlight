/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * collection.h: different types of collections
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __MOON_COLLECTION_H__
#define __MOON_COLLECTION_H__

#include <glib.h>

#include "dependencyobject.h"
#include "eventargs.h"
#include "point.h"


//
// Collection: provides a collection that we can monitor for
// changes.   We expose this collection in a few classes to
// the managed world, and when a change happens we get a
// chance to reflect the changes
//
class Collection : public DependencyObject {
 protected:
	GPtrArray *array;
	int generation;
	
	void EmitChanged (CollectionChangedAction action, Value *new_value, Value *old_value, int index);
	
	virtual bool CanAdd (Value value) { return true; }
	virtual void AddedToCollection (Value *value) {}
	virtual void RemovedFromCollection (Value *value) {}
	
	Collection ();
	virtual ~Collection ();
	virtual void Dispose ();
	
 public:
 	/* @PropertyType=gint32 */
	static DependencyProperty *CountProperty;
	
	DependencyObject *closure;
	
	virtual Type::Kind GetObjectType () = 0;
	virtual Type::Kind GetElementType () = 0;
	
	virtual Value *GetValue (DependencyProperty *property);
	
	int Generation () { return generation; }
	GPtrArray *Array () { return array; }
	
	int GetCount () { return array->len; }
	
	virtual int Add (Value value);
	virtual void Clear ();
	bool Contains (Value value);
	int IndexOf (Value value);
	virtual bool Insert (int index, Value value);
	bool Remove (Value value);
	bool RemoveAt (int index);
	
	Value *GetValueAt (int index);
	Value *SetValueAt (int index, Value value);
};

class DependencyObjectCollection : public Collection {
 protected:
	virtual bool CanAdd (Value value) { return value.AsDependencyObject ()->GetLogicalParent () == NULL; }
	virtual void AddedToCollection (Value *value);
	virtual void RemovedFromCollection (Value *value);
	
	virtual ~DependencyObjectCollection () {}
	
 public:
	/* @GenerateCBinding */
	DependencyObjectCollection () {}
	
	virtual Type::Kind GetObjectType () { return Type::DEPENDENCY_OBJECT_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::DEPENDENCY_OBJECT; }
	
	virtual DependencyObject *SetValueAt (int index, DependencyObject *obj);
	
	virtual void SetSurface (Surface *surface);
	
	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *args);
	virtual void UnregisterAllNamesRootedAt (NameScope *from_ns);
	virtual void RegisterAllNamesRootedAt (NameScope *to_ns);
	
	void MergeNames (DependencyObject *new_obj);
};

class DoubleCollection : public Collection {
 protected:
	virtual ~DoubleCollection () {}
	
 public:
	/* @GenerateCBinding */
	DoubleCollection () {}
	
	virtual Type::Kind GetObjectType () { return Type::DOUBLE_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::DOUBLE; }
};

class PointCollection : public Collection {
 protected:
	virtual ~PointCollection () {}
	
 public:
	/* @GenerateCBinding */
	PointCollection () {}
	
	virtual Type::Kind GetObjectType () { return Type::POINT_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::POINT; }
};

class CollectionIterator {
 public:
	CollectionIterator (Collection *c)
	{
		generation = c->Generation ();
		collection = c;
		index = -1;
	}
	
	Collection *collection;
	int generation;
	int index;
};

class TriggerCollection : public DependencyObjectCollection {
 protected:
	virtual void AddedToCollection (Value *value);
	virtual void RemovedFromCollection (Value *value);
	
	virtual ~TriggerCollection () {}
	
 public:
	/* @GenerateCBinding */
	TriggerCollection () {}
	
	virtual Type::Kind GetObjectType () { return Type::TRIGGER_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::EVENTTRIGGER; }
};

class TriggerActionCollection : public DependencyObjectCollection {
 protected:
	virtual ~TriggerActionCollection () {}

 public:
	/* @GenerateCBinding */
	TriggerActionCollection () {}
	virtual Type::Kind GetObjectType () { return Type::TRIGGERACTION_COLLECTION; }
	/* this may seem wrong, but it's what the TriggerActionCollection mandates */
	virtual Type::Kind GetElementType () { return Type::BEGINSTORYBOARD; }
};

class ResourceDictionary : public DependencyObjectCollection {
 protected:
	virtual ~ResourceDictionary () {}
	
 public:
	/* @GenerateCBinding */
	ResourceDictionary () {}
	
	virtual Type::Kind GetObjectType () { return Type::RESOURCE_DICTIONARY; }
	// XXX FIXME this should be "object"
	virtual Type::Kind GetElementType () { return Type::DEPENDENCY_OBJECT; }
};

class Inlines : public DependencyObjectCollection {
 protected:
	virtual ~Inlines () {}

 public:
	/* @GenerateCBinding */
	Inlines () {}
	virtual Type::Kind GetObjectType () { return Type::INLINES; }
	virtual Type::Kind GetElementType () { return Type::INLINE; }
};

class UIElementCollection : public DependencyObjectCollection {
 protected:
	virtual void AddedToCollection (Value *value);
	virtual void RemovedFromCollection (Value *value);
	
	virtual ~UIElementCollection ();
	virtual void Dispose ();
	
 public:
	GPtrArray *z_sorted;
	
	/* @GenerateCBinding */
	UIElementCollection ();
	
	virtual Type::Kind GetObjectType () { return Type::UIELEMENT_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::UIELEMENT; }
	
	virtual bool Insert (int index, Value value);
	virtual void Clear ();
	
	void ResortByZIndex ();
};

G_BEGIN_DECLS

Collection *collection_new (Type::Kind kind);

Type::Kind collection_get_element_type (Collection *collection);
int collection_get_count (Collection *collection);

int collection_add (Collection *collection, Value *value);
void collection_clear (Collection *collection);
bool collection_contains (Collection *collection, Value *value);
int collection_index_of (Collection *collection, Value *value);
bool collection_insert (Collection *collection, int index, Value *value);
bool collection_remove (Collection *collection, Value *value);
bool collection_remove_at (Collection *collection, int index);

Value *collection_get_value_at (Collection *collection, int index);
void collection_set_value_at (Collection *collection, int index, Value *value);

CollectionIterator *collection_get_iterator (Collection *collection);
int collection_iterator_next (CollectionIterator *iterator);
bool collection_iterator_reset (CollectionIterator *iterator);
void collection_iterator_destroy (CollectionIterator *iterator);
Value *collection_iterator_get_current (CollectionIterator *iterator, int *error);

DoubleCollection *double_collection_from_str (const char *s);
PointCollection *point_collection_from_str (const char *s);
GArray* double_garray_from_str   (const char *s, gint max);

G_END_DECLS

#endif /* __MOON_COLLECTION_H__ */
