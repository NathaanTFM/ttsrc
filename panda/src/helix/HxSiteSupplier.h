// Filename: HxSiteSupplier.h
// Created by:  jjtaylor (29Jan04)
//
////////////////////////////////////////////////////////////////////
//
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////
#ifndef HXSITESUPPLIER_H
#define HXSITESUPPLIER_H

////////////////////////////////////////////////////////////////////
// Panda Header Files
////////////////////////////////////////////////////////////////////
#include "pandabase.h"
#include "pmap.h"
#include "texture.h"
#include "fivemmap.h"

////////////////////////////////////////////////////////////////////
// Class: HxSiteSupplier
// Purpose: This is a derived class from the IHXSiteSupplier
//          base interface. This class is meant to create or 
//          remove Helix Sites.
////////////////////////////////////////////////////////////////////
class HxSiteSupplier : public IHXSiteSupplier {
public:
  HxSiteSupplier(IUnknown* unknown, Texture* tex);
  ~HxSiteSupplier();

////////////////////////////////////////////////////////////////////
// IHUnkown Interface Methods Prototypes
// Purpose: Implements the basic COM interface methods for reference
//          coutning and querying of related interfaces.
////////////////////////////////////////////////////////////////////
  STDMETHOD(QueryInterface) (THIS_ REFIID id, void** interface_obj);
  STDMETHOD_(ULONG32,AddRef) (THIS);
  STDMETHOD_(ULONG32,Release) (THIS);

////////////////////////////////////////////////////////////////////
// IHXClientAdviseSink Interface Methods Prototypes
// Purpose: Implements the necessary interface methods that may
//          be called when a site must be created or closed.
////////////////////////////////////////////////////////////////////
  STDMETHOD(SitesNeeded) (THIS_ UINT32 request_id, IHXValues* site_props);
  STDMETHOD(SitesNotNeeded) (THIS_ UINT32 request_id);
  STDMETHOD(BeginChangeLayout) (THIS);
  STDMETHOD(DoneChangeLayout) (THIS);
private:
  LONG32 _ref_count;
  IHXSiteManager* _site_manager;
  IHXCommonClassFactory* _ccf;
  IUnknown* _unknown;
  FiveMinuteMap _created_sites;
  UCHAR* _dest_buffer;
  Texture* _texture;
};
#endif 
