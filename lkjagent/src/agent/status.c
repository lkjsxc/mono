#include "agent/status.h"

result_t agent_status_change(agent_t* agent, json_value_t* status_change) {
    if (!status_change || status_change->type != JSON_TYPE_STRING) {
        return RESULT_OK; // Nothing to process
    }

    const char* new_status = status_change->u.string_value->data;

    if (strcmp(new_status, "thinking") == 0) {
        agent->status = AGENT_STATUS_THINKING;
    } else if (strcmp(new_status, "paging") == 0) {
        agent->status = AGENT_STATUS_PAGING;
    } else if (strcmp(new_status, "evaluating") == 0) {
        agent->status = AGENT_STATUS_EVALUATING;
    } else if (strcmp(new_status, "executing") == 0) {
        agent->status = AGENT_STATUS_EXECUTING;
    } else {
        RETURN_ERR("Invalid agent status in response");
    }

    return RESULT_OK;
}
