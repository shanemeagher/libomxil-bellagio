/**
  src/omx_reference_resource_manager.c

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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/media.h>

#include "omx_samsung_resource_manager.h"
#include "base/omx_base_component.h"
#include "queue.h"

#ifndef MAX_INT
#define MAX_INT  0x7fffffff
#endif

static int globalTimestamp = 0;

static int SRM_PopulateMediaEntityList(int media_devnum);

int media_enumerate_entities(int dev_num);

/**
 * The list of available media entities in the system.
 */
MediaEntityListType *mediaEntityList = NULL;

/**
 * This function initializes the Resource manager.
 * Currently it just initializes the list of available media entities.
 */
OMX_ERRORTYPE RM_Init(void)
{
	int i;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	globalIndex = 0;
	listOfcomponentRegistered = calloc(1, sizeof(struct NameIndexType) * MAX_COMPONENT_TYPES_HANDLED);
	for (i = 0; i < MAX_COMPONENT_TYPES_HANDLED; i++) {
		listOfcomponentRegistered[i].index = -1;
		listOfcomponentRegistered[i].component_name = NULL;
	}

	globalComponentList = calloc(sizeof(ComponentListType*), MAX_COMPONENT_TYPES_HANDLED);
	globalWaitingComponentList = calloc(sizeof(ComponentListType*), MAX_COMPONENT_TYPES_HANDLED);

	SRM_PopulateMediaEntityList(0);

	SRM_printMediaEntityList(mediaEntityList, 0);

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/** This function is called during initialization by any component interested in be
 * handled by the internal resource manager
 */
OMX_ERRORTYPE RM_RegisterComponent(char *name, int max_components)
{
	int i = 0;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	while (listOfcomponentRegistered[i].component_name != NULL) {
		if (!strcmp(listOfcomponentRegistered[i].component_name, name)) {
			DEBUG(DEB_LEV_FUNCTION_NAME, "In %s component already registered\n", __func__);
			return OMX_ErrorNone;
		}
		i++;
	}
	listOfcomponentRegistered[i].component_name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
	if (listOfcomponentRegistered[i].component_name == NULL) {
		return OMX_ErrorInsufficientResources;
	}
	strcpy(listOfcomponentRegistered[i].component_name, name);
	listOfcomponentRegistered[i].component_name[strlen(name)] = '\0';
	listOfcomponentRegistered[i].index = globalIndex;
	listOfcomponentRegistered[i].max_components = max_components;
	globalIndex++;
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function de-initializes the resource manager.
 * In the current implementation its responsibility is to clean up any queue
 * that can be left pending at the end of usage. With a correct use of the
 * resource manager it won't happen, but it is safer to clean up everything
 * this these lists are global and alive for all the life of IL client,
 * beyond the usual OMX_Init - Deinit scope.
 */
OMX_ERRORTYPE RM_Deinit()
{
	int i = 0;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

	while(globalComponentList[i] != NULL) {
		clearList(&globalComponentList[i]);
		clearList(&globalWaitingComponentList[i]);
		i++;
	}

	free(globalComponentList);
	free(globalWaitingComponentList);

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function adds a new element to a given list.
 * If it does not yet exists, this function also allocates the list.
 */
OMX_ERRORTYPE addElemToList(ComponentListType **list, OMX_COMPONENTTYPE *openmaxStandComp,
			    int index, OMX_BOOL bIsWaiting)
{
	ComponentListType *componentTemp;
	ComponentListType *componentNext;
	omx_base_component_PrivateType* omx_base_component_Private;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s is waiting %i\n", __func__, bIsWaiting);

	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
	if (!*list) {
		*list = malloc(sizeof(ComponentListType));
		if (!bIsWaiting) {
			globalComponentList[index] = *list;
		} else {
			globalWaitingComponentList[index] = *list;
		}
		if (!*list) {
			DEBUG(DEB_LEV_ERR, "In %s OMX_ErrorInsufficientResources\n", __func__);
			return OMX_ErrorInsufficientResources;
		}
		(*list)->openmaxStandComp = openmaxStandComp;
		(*list)->timestamp = globalTimestamp;
		globalTimestamp++;
		(*list)->nGroupPriority = omx_base_component_Private->nGroupPriority;
		(*list)->next = NULL;
		DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
		return OMX_ErrorNone;
	}
	componentTemp = *list;
	while(componentTemp->next) {
		componentTemp = componentTemp->next;
	}
	componentNext = malloc(sizeof(ComponentListType));
	if (!componentNext) {
		DEBUG(DEB_LEV_ERR, "In %s OMX_ErrorInsufficientResources\n", __func__);
		return OMX_ErrorInsufficientResources;
	}
	componentTemp->next = componentNext;
	componentNext->next = NULL;
	componentNext->openmaxStandComp = openmaxStandComp;
	componentNext->timestamp = globalTimestamp;
	globalTimestamp++;
	componentNext->nGroupPriority = omx_base_component_Private->nGroupPriority;
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function removes the given element from the list, if present.
 * If the list is empty, this function cleans up everything.
 */
OMX_ERRORTYPE removeElemFromList(ComponentListType **list,
				 OMX_COMPONENTTYPE *openmaxStandComp)
{
	ComponentListType *componentTemp;
	ComponentListType *componentPrev;
	OMX_BOOL bFound = OMX_FALSE;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s list %p\n", __func__, *list);
	if (!*list) {
		DEBUG(DEB_LEV_ERR, "In %s, the resource manager is not initialized\n", __func__);
		return OMX_ErrorUndefined;
	}
	componentTemp = *list;
	componentPrev = *list;
	while(componentTemp) {
		if (componentTemp->openmaxStandComp == openmaxStandComp) {
			if (componentTemp == *list) {
				*list = (*list)->next;
				free(componentTemp);
			} else {
				componentPrev->next = componentTemp->next;
				free(componentTemp);
			}
			 bFound = OMX_TRUE;
			 break;
		} else {
			if (componentTemp != *list) {
				componentPrev = componentPrev->next;
			}
			componentTemp = componentTemp->next;
		}
	}
	if(!bFound) {
		DEBUG(DEB_LEV_ERR, "In %s, the specified component does not exist\n", __func__);
		return OMX_ErrorComponentNotFound;
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;

}

/**
 * This function returns the number of elements present in the
 * list. If the list does not exists, this function returns 0 elements
 * without further warnings
 */
int numElemInList(ComponentListType *list)
{
	ComponentListType *componentTemp;
	int numElem = 0;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	if (!list) {
		DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s, no list no elements\n", __func__);
		return 0;
	}
	componentTemp = list;
	while(componentTemp) {
		numElem++;
		componentTemp = componentTemp->next;
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return numElem;
}

/**
 * This function deallocate any remaining element in a list
 * and dispose it
 */
OMX_ERRORTYPE clearList(ComponentListType **list)
{
	ComponentListType *componentTemp;
	ComponentListType *componentPrev;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	if (!*list) {
		DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, no list no elements\n", __func__);
		return OMX_ErrorNone;
	}
	componentTemp = *list;
	while(componentTemp) {
		componentPrev = componentTemp;
		componentTemp = componentTemp->next;
		free(componentPrev);
	}
	*list = NULL;
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This debug function is capable of printing the full list
 * actually stored
 */
void RM_printList(ComponentListType *list, int viewFlag)
{
	ComponentListType *componentTemp = list;
	omx_base_component_PrivateType* omx_base_component_Private;
	int index;

	if (!list) {
		printf("The list is empty\n");
		return;
	}
	index = 0;
	while (componentTemp) {
		omx_base_component_Private = (omx_base_component_PrivateType*)componentTemp->openmaxStandComp->pComponentPrivate;
		if ((viewFlag & RM_SHOW_NAME) == RM_SHOW_NAME) {
			printf("Name %s ", omx_base_component_Private->name);
		}
		if ((viewFlag & RM_SHOW_ADDRESS) == RM_SHOW_ADDRESS) {
			printf("Address %p ", componentTemp->openmaxStandComp);
		}
		printf("\n");
		index++;
		componentTemp = componentTemp->next;
	}
}

/**
 *
 */
void SRM_printMediaEntityList(MediaEntityListType *list, int viewFlag)
{
	MediaEntityListType *entity = list;

	if (!list) {
		printf("The list is empty\n");
		return;
	}

	while (entity) {
		printf("Entity: %s, device node: %s, type: %#x\n",
		       entity->name, entity->devname, entity->type);
		entity = entity->next;
	}
}

/**
 * This function returns the number of components that have a lower priority
 * than the value specified, and the lowest among all possibles.
 * If the number returned is 0, no component is preemptable. if it is 1 or more,
 * the oldest_component_preemptable will contain the reference to the preemptable
 * component with the oldest time stamp.
 */
int searchLowerPriority(ComponentListType *list, int current_priority,
			ComponentListType **oldest_component_preemptable)
{
	ComponentListType *componentTemp;
	ComponentListType *componentCandidate;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	int nComp = 0;
	if (!list) {
		DEBUG(DEB_LEV_ERR, "In %s no list\n", __func__);
		return OMX_ErrorUndefined;
	}
	componentTemp = list;
	componentCandidate = NULL;
	while (componentTemp) {
		if (componentTemp->nGroupPriority > current_priority) {
			nComp++;
		}
		if (nComp > 0) {
			if (componentCandidate) {
				if (componentCandidate->timestamp > componentTemp->timestamp) {
					componentCandidate = componentTemp;
				}
			} else {
				componentCandidate = componentTemp;
			}
		}
		componentTemp = componentTemp->next;
	}
	*oldest_component_preemptable = componentCandidate;
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return nComp;
}

/**
 * This function tries to preempt the given component, that has been detected as
 * the candidate by the default policy defined in the OpenMAX spec.
 */
OMX_ERRORTYPE preemptComponent(OMX_COMPONENTTYPE *openmaxStandComp)
{
	OMX_ERRORTYPE err;
	omx_base_component_PrivateType* omx_base_component_Private = openmaxStandComp->pComponentPrivate;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

	if (omx_base_component_Private->state == OMX_StateIdle) {
        (*(omx_base_component_Private->callbacks->EventHandler))
          (openmaxStandComp, omx_base_component_Private->callbackData,
            OMX_EventError, OMX_ErrorResourcesLost, 0, NULL);
        err = OMX_SendCommand(openmaxStandComp, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (err != OMX_ErrorNone) {
        	DEBUG(DEB_LEV_ERR, "In %s, the state cannot be changed\n", __func__);
        	return OMX_ErrorUndefined;
        }
    } else if ((omx_base_component_Private->state == OMX_StateExecuting) ||
	       (omx_base_component_Private->state == OMX_StatePause)) {
    	// TODO fill also this section that cover the preemption of a running component
    	// send OMX_ErrorResourcesPreempted
    	// change state to Idle
    	// send OMX_ErrorResourcesLost
    	// change state to Loaded
    }
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function is executed by a component when it changes state from Loaded to Idle.
 * If it return ErrorNone the resource is granted and it can transit to Idle.
 * In case the resource is already busy, the resource manager preempt another component
 * with a lower priority and a oldest time flag if it exists. Differently it returns
 * OMX_ErrorInsufficientResources
 */
OMX_ERRORTYPE RM_getResource(OMX_COMPONENTTYPE *openmaxStandComp)
{
	ComponentListType *componentCandidate;
	omx_base_component_PrivateType* omx_base_component_Private;
	int candidates;
	OMX_ERRORTYPE err;
	int i = 0;
	int indexComponent = -1;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
	while(listOfcomponentRegistered[i].component_name != NULL ) {
		if (!strcmp(listOfcomponentRegistered[i].component_name, omx_base_component_Private->name)) {
			// found component in the list of the resource manager
			indexComponent = listOfcomponentRegistered[i].index;
			break;
		}
		i++;
	}
	if (indexComponent <0) {
		// No resource to be handled
		DEBUG(DEB_LEV_ERR, "In %s No resource to be handled\n", __func__);
		return OMX_ErrorNone;
	}
	if (numElemInList(globalComponentList[indexComponent]) >= listOfcomponentRegistered[i].max_components) {
		candidates = searchLowerPriority(globalComponentList[indexComponent], omx_base_component_Private->nGroupPriority, &componentCandidate);
		if (candidates) {
			DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s candidates %i winner %p\n", __func__, candidates, componentCandidate->openmaxStandComp);
			err = preemptComponent(componentCandidate->openmaxStandComp);
			if (err != OMX_ErrorNone) {
				DEBUG(DEB_LEV_ERR, "In %s the component cannot be preempted\n", __func__);
				return OMX_ErrorInsufficientResources;
			} else {
				err = removeElemFromList(&globalComponentList[indexComponent], componentCandidate->openmaxStandComp);
				err = addElemToList(&globalComponentList[indexComponent], openmaxStandComp, indexComponent, OMX_FALSE);
				if (err != OMX_ErrorNone) {
					DEBUG(DEB_LEV_ERR, "In %s memory error\n", __func__);
					return OMX_ErrorInsufficientResources;
				}
			}
		} else {
			DEBUG(DEB_LEV_SIMPLE_SEQ, "Out of %s with insufficient resources\n", __func__);
			return OMX_ErrorInsufficientResources;
		}

	} else {
		err = addElemToList(&globalComponentList[indexComponent], openmaxStandComp, indexComponent, OMX_FALSE);
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function is called by a component when it transit from Idle to Loaded and can release
 * its used resource handled by the resource manager
 */
OMX_ERRORTYPE RM_releaseResource(OMX_COMPONENTTYPE *openmaxStandComp)
{
	omx_base_component_PrivateType* omx_base_component_Private;
	OMX_COMPONENTTYPE *openmaxWaitingComp;
	OMX_ERRORTYPE err;

	int i = 0;
	int indexComponent = -1;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;

	while(listOfcomponentRegistered[i].component_name != NULL ) {
		if (!strcmp(listOfcomponentRegistered[i].component_name, omx_base_component_Private->name)) {
			// found component in the list of the resource manager
			indexComponent = listOfcomponentRegistered[i].index;
			break;
		}
		i++;
	}
	if (indexComponent <0) {
		// No resource to be handled
		DEBUG(DEB_LEV_ERR, "In %s No resource to be handled\n", __func__);
		return OMX_ErrorNone;
	}
	if (!globalComponentList[indexComponent]) {
		DEBUG(DEB_LEV_ERR, "In %s, the resource manager is not initialized\n", __func__);
		return OMX_ErrorUndefined;
	}
	err = removeElemFromList(&globalComponentList[indexComponent], openmaxStandComp);
	if (err != OMX_ErrorNone) {
		DEBUG(DEB_LEV_ERR, "In %s, the resource cannot be released\n", __func__);
		return OMX_ErrorUndefined;
	}
	if(numElemInList(globalWaitingComponentList[indexComponent])) {
		openmaxWaitingComp = globalWaitingComponentList[indexComponent]->openmaxStandComp;
		removeElemFromList(&globalWaitingComponentList[indexComponent], openmaxWaitingComp);
        err = OMX_SendCommand(openmaxWaitingComp, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if (err != OMX_ErrorNone) {
        	DEBUG(DEB_LEV_ERR, "In %s, the state cannot be changed\n", __func__);
        }
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function adds the given component to the waiting queue for
 * the given resource. When a resource becomes available through the
 * RM_releaseResource function the first element in the queue is taken
 * off the list and it receives the resource just released.
 */
OMX_ERRORTYPE RM_waitForResource(OMX_COMPONENTTYPE *openmaxStandComp)
{
	omx_base_component_PrivateType* omx_base_component_Private;

	int i = 0;
	int indexComponent = -1;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;

	while(listOfcomponentRegistered[i].component_name != NULL ) {
		if (!strcmp(listOfcomponentRegistered[i].component_name, omx_base_component_Private->name)) {
			// found component in the list of the resource manager
			indexComponent = listOfcomponentRegistered[i].index;
			break;
		}
		i++;
	}
	if (indexComponent <0) {
		// No resource to be handled
		DEBUG(DEB_LEV_ERR, "In %s No resource to be handled\n", __func__);
		return OMX_ErrorNone;
	}

	addElemToList(&globalWaitingComponentList[indexComponent], openmaxStandComp,
		      listOfcomponentRegistered[i].index, OMX_TRUE);

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/**
 * This function removes a component from the waiting queue
 * if the IL client decides that the component should not wait any
 * more for the resource
 */
OMX_ERRORTYPE RM_removeFromWaitForResource(OMX_COMPONENTTYPE *openmaxStandComp)
{
	omx_base_component_PrivateType* omx_base_component_Private;
	int i = 0;
	int indexComponent = -1;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;

	while(listOfcomponentRegistered[i].component_name != NULL ) {
		if (!strcmp(listOfcomponentRegistered[i].component_name, omx_base_component_Private->name)) {
			// found component in the list of the resource manager
			removeElemFromList(&globalComponentList[indexComponent], openmaxStandComp);
			break;
		}
		i++;
	}
	if (indexComponent <0) {
		// No resource to be handled
		DEBUG(DEB_LEV_ERR, "In %s No resource to be handled\n", __func__);
		return OMX_ErrorNone;
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return OMX_ErrorNone;
}

void media_enumerate_links(int fd, int num_links, int entity_num)
{
	struct media_links_enum links_enum;
	struct media_pad_desc pads[32];
	struct media_link_desc links[32];
	int i, err;

	if (num_links <= 0)
		return;

	memset(&links_enum, 0, sizeof(links_enum));

	links_enum.links = links;
	links_enum.pads = pads;
	links_enum.entity = entity_num;

	err = ioctl(fd, MEDIA_IOC_ENUM_LINKS, &links_enum);
	if (err) {
		perror("ioctl MEDIA_IOC_ENUM_LINKS");
		return;
	}

	for (i = 0; i < num_links; i++) {
		struct media_pad_desc *src_pad = &links[i].source;
		struct media_pad_desc *sink_pad = &links[i].sink;
		fprintf(stderr,
			"[entity %.2d, pad %d] ==> [entity %.2d, pad %d], "
			"link flags: 0x%X\n",
			src_pad->entity, src_pad->index,
			sink_pad->entity, sink_pad->index,
			links[i].flags);
	}
}

static int SRM_PopulateMediaEntityList(int dev_num)
{
	struct media_entity_desc me_desc;
	struct stat devstat;
	int entity_count = 0;
	char devname[64];
	char sysname[32];
	char target[1024];
	int err = 0;

	snprintf(devname, 64, "/dev/media%d", dev_num);
	DEBUG(DEB_LEV_FUNCTION_NAME, "Enumerating entities on %s...\n", devname);

	int fd = open(devname, O_RDWR, 0);
	if (fd < 0) {
		perror("/dev/media? open");
		return OMX_ErrorHardware;
	}

	memset(&me_desc, 0, sizeof(me_desc));
	me_desc.id = 0;

	while(1) {
		MediaEntityListType *entityTmp;
		MediaEntityListType **entityNext;
		int ret;
		char *p;

		me_desc.id |= MEDIA_ENT_ID_FLAG_NEXT;
		err = ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &me_desc);
		if (err)
			break;

		entity_count++;

		if ((me_desc.type != MEDIA_ENT_T_DEVNODE_V4L &&
		     me_desc.type != MEDIA_ENT_T_V4L2_SUBDEV))
			continue;

		sprintf(sysname, "/sys/dev/char/%u:%u",
			me_desc.v4l.major, me_desc.v4l.minor);
		ret = readlink(sysname, target, sizeof(target));
		if (ret < 0)
			continue;

		target[ret] = '\0';
		p = strrchr(target, '/');
		if (p == NULL)
			continue;

		entityTmp = calloc(sizeof(MediaEntityListType), 1);
		if (!entityTmp) {
			DEBUG(DEB_LEV_ERR, "In %s OMX_ErrorInsufficientResources\n", __func__);
			return OMX_ErrorInsufficientResources;
		}

		sprintf(entityTmp->devname, "/dev/%s", ++p);
		ret = stat(entityTmp->devname, &devstat);
		if (ret < 0) {
			free(entityTmp);
			continue;
		}

		/* TODO: use libudev rather than just sysfs */
		if (major(devstat.st_rdev) != me_desc.v4l.major ||
		    minor(devstat.st_rdev) != me_desc.v4l.minor)
			entityTmp->devname[0] = '\0';

		strncpy(entityTmp->name, me_desc.name, sizeof(entityTmp->name));
		entityTmp->type = me_desc.type;

		DEBUG(DEB_LEV_FUNCTION_NAME,
		      "Entity: %d: %s\ntype: 0x%X, devnode: %s\n",
		      me_desc.id, me_desc.name, me_desc.type, entityTmp->devname);

		if (!mediaEntityList) {
			mediaEntityList = entityTmp;
			entityNext = &mediaEntityList->next;
		} else {
			*entityNext = entityTmp;
			entityNext = &((*entityNext)->next);
		}
	};

	DEBUG(DEB_LEV_FUNCTION_NAME, "Found %d entities\n", entity_count);
	close(fd);

	return OMX_ErrorNone;
}

/**
 * This functions returns first available FIMC mem-to-mem video node
 * which is used by node more than max_use_count components.
 */
OMX_ERRORTYPE SRM_GetFreeVideoM2MDevName(char *devname, int max_use_count)
{
	MediaEntityListType *entity = mediaEntityList;
	MediaEntityListType *match = NULL;
	int min_use_count = MAX_INT;

	if (!entity) {
		printf("The list is empty\n");
		return OMX_ErrorUndefined;
	}

	while (entity) {
		if (strstr(entity->name, ".m2m") != NULL &&
		    entity->use_count <= max_use_count &&
		    entity->use_count < min_use_count) {
			match = entity;
			min_use_count = entity->use_count;
		}
		entity = entity->next;
	}

	/*
	 * TODO: Also need to check if the m2m node is not available due
	 * to capture node being used on same hardware instance.
	 */
	if (match) {
		strncpy(devname, match->devname, 32);
		match->use_count++;

		DEBUG(DEB_LEV_FUNCTION_NAME,
		      "\nEntity: %s, device node: %s, type: %#x, use_count: %d\n",
		      match->name, match->devname, match->type,
		      match->use_count);

		return OMX_ErrorNone;
	}

	return OMX_ErrorInsufficientResources;
}


OMX_ERRORTYPE SRM_PutVideoNode(char *devname)
{
	MediaEntityListType *entity = mediaEntityList;

	if (!entity)
		return OMX_ErrorNone;

	while (entity) {
		if (strcmp(entity->devname, devname) != 0) {
			entity = entity->next;
			continue;
		}

		entity->use_count--;
		DEBUG(DEB_LEV_FUNCTION_NAME,
		      "Entity: %s, device node: %s, type: %#x, use_count: %d\n",
		      entity->name, entity->devname, entity->type,
		      entity->use_count);

		return OMX_ErrorNone;
	}

	return OMX_ErrorUndefined;
}
