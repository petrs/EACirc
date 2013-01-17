#include "IProject.h"
#include "CommonFnc.h"
#include "estream/EstreamProject.h"
#include "sha3/Sha3Project.h"

IProject::IProject(int type) : m_type(type) {}

IProject::~IProject() {}

int IProject::loadProjectConfiguration(TiXmlNode *pRoot) {
    return STAT_OK;
}

TiXmlNode* IProject::saveProjectState() const {
    TiXmlElement* pNode = new TiXmlElement("project");
    pNode->SetAttribute("type",toString(m_type).c_str());
    pNode->SetAttribute("description",shortDescription().c_str());
    return pNode;
}

int IProject::loadProjectState(TiXmlNode *pRoot) {
    int loadedType = atoi(getXMLElementValue(pRoot,"@type").c_str());
    if ( loadedType != m_type) {
        mainLogger.out() << "error: Incompatible project type." << endl;
        mainLogger.out() << "       required: " << m_type << "  given: " << loadedType << endl;
        return STAT_INCOMPATIBLE_PARAMETER;
    }
    return STAT_OK;
}

IProject* IProject::getProject(int projectType) {
    IProject* project = NULL;
    switch (projectType) {
    case PROJECT_ESTREAM:
        project = new EstreamProject();
        break;
    case PROJECT_SHA3:
        project = new Sha3Project();
        break;
    default:
        mainLogger.out() << "error: Cannot initialize project - unknown type (" << projectType << ")." << endl;
        project = NULL;
        break;
    }
    mainLogger.out() << "info: Project successfully initialized. (" << project->shortDescription() << ")" << endl;
    return project;
}
