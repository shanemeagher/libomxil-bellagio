/**
  src/omx_reference_resource_manager.h

  This simple resource manager emulates the behavior of a real RM.
	It applies the rules defined in the OpenMAX spec. It can be replaced in
	the future by a real system.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
  Copyright (C) 2011 Samsung Electronics Co., Ltd.

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef _OMX_SLP_RESOURCE_MANAGER_H_
#define _OMX_SLP_RESOURCE_MANAGER_H_

#define MAX_COMPONENT_TYPES_HANDLED	500
#define MAX_MEDIA_ENTITIES_HANDLED	60

#include "omxcore.h"


typedef struct ComponentListType ComponentListType;
struct ComponentListType {
	OMX_COMPONENTTYPE *openmaxStandComp;
	OMX_U32 nGroupPriority;
	OMX_U32 timestamp;
	ComponentListType *next;
};

typedef struct NameIndexType NameIndexType;
struct NameIndexType {
	char *component_name;
	int index;
	int max_components;
};

typedef enum V4L2DevnodeType V4L2DevnodeType;
enum V4L2DevnodeType {
	V4L2_DEVICE_TYPE_VIDEO_CAPTURE,
	V4L2_DEVICE_TYPE_VIDEO_OUTPUT,
	V4L2_DEVICE_TYPE_VIDEO_MEM2MEM,
	V4L2_DEVICE_TYPE_VIDEO_SUBDEV,
	V4L2_DEVICE_TYPE_VIDEO_SENSOR_SUBDEV,
};

typedef struct MediaEntityListType MediaEntityListType;
struct MediaEntityListType {
	OMX_COMPONENTTYPE *openmaxStandComp;
	char name[32];
	char devname[32];
	unsigned int type;
	int use_count;
	MediaEntityListType *next;
};

typedef struct V4L2ResourceType V4L2ResourceType;
struct V4L2ResourceType {
	OMX_COMPONENTTYPE *openmaxStandComp;
	V4L2DevnodeType devType;
	int fd;
};

int globalIndex;
NameIndexType *listOfcomponentRegistered;
ComponentListType **globalComponentList;
ComponentListType **globalWaitingComponentList;

OMX_ERRORTYPE RM_RegisterComponent(char *name, int max_components);
OMX_ERRORTYPE addElemToList(ComponentListType **list, OMX_COMPONENTTYPE *openmaxStandComp, int index, OMX_BOOL bIsWaiting);
OMX_ERRORTYPE removeElemFromList(ComponentListType **list, OMX_COMPONENTTYPE *openmaxStandComp);
int numElemInList(ComponentListType *list);
OMX_ERRORTYPE clearList(ComponentListType **list);

/** Debug flags
 */
#define RM_SHOW_NAME	0x01
#define RM_SHOW_ADDRESS	0x02

OMX_ERRORTYPE RM_Init();
OMX_ERRORTYPE RM_Deinit();
OMX_ERRORTYPE RM_getResource(OMX_COMPONENTTYPE *openmaxStandComp); //, V4L2ResourceType resType);
OMX_ERRORTYPE RM_releaseResource(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE RM_waitForResource(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE RM_removeFromWaitForResource(OMX_COMPONENTTYPE *openmaxStandComp);
void RM_printList(ComponentListType *list, int viewFlag);

void SRM_printMediaEntityList(MediaEntityListType *list, int viewFlag);
OMX_ERRORTYPE SRM_GetFreeVideoM2MDevName(char *devname, int max_use_count);

#endif /* _OMX_SLP_RESOURCE_MANAGER_H_ */
