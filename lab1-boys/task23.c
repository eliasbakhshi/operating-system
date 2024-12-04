#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int locateEvictionIndex(int *slots, int slotLimit, int *accessSequence, int currentStep, int sequenceLength) {
    int farthestAccess = -1;
    int evictionSlot = -1;

    for (int slot = 0; slot < slotLimit; slot++) {
        int step;
        for (step = currentStep + 1; step < sequenceLength; step++) {
            if (slots[slot] == accessSequence[step]) {
                if (step > farthestAccess) {
                    farthestAccess = step;
                    evictionSlot = slot;
                }
                break;
            }
        }
        if (step == sequenceLength) {
            return slot;
        }
    }

    return (evictionSlot == -1) ? 0 : evictionSlot;
}

bool slotContainsPage(int *slots, int slotLimit, int page) {
    for (int slot = 0; slot < slotLimit; slot++) {
        if (slots[slot] == page) {
            return true;
        }
    }
    return false;
}

int simulateOptimalAlgorithm(int *accessSequence, int sequenceLength, int slotLimit) {
    int *slots = (int *)malloc(slotLimit * sizeof(int));
    for (int i = 0; i < slotLimit; i++) {
        slots[i] = -1;
    }

    int faultCount = 0;

    for (int step = 0; step < sequenceLength; step++) {
        int pageNumber = accessSequence[step];

        if (!slotContainsPage(slots, slotLimit, pageNumber)) {
            faultCount++;
            if (step >= slotLimit) {
                int evictionIndex = locateEvictionIndex(slots, slotLimit, accessSequence, step, sequenceLength);
                slots[evictionIndex] = pageNumber;
            } else {
                slots[step] = pageNumber;
            }
        }
    }

    free(slots);
    return faultCount;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s num_slots slot_size trace_file\n", argv[0]);
        return 1;
    }

    int numSlots = atoi(argv[1]);
    int slotSize = atoi(argv[2]);
    const char *traceFile = argv[3];

    printf("No physical slots = %d, slot size = %d\n", numSlots, slotSize);
    printf("Reading memory trace from %s...\n", traceFile);

    FILE *trace = fopen(traceFile, "r");
    if (!trace) {
        perror("Error opening file");
        return 1;
    }

    int *sequence = (int *)malloc(100000 * sizeof(int));
    int sequenceLength = 0;
    unsigned int memoryAddress;

    while (fscanf(trace, "%u", &memoryAddress) == 1) {
        sequence[sequenceLength++] = memoryAddress / slotSize;
    }

    fclose(trace);
    printf("Read %d memory accesses\n", sequenceLength);

    int faultResult = simulateOptimalAlgorithm(sequence, sequenceLength, numSlots);

    printf("Result: %d page faults\n", faultResult);

    free(sequence);
    return 0;
}
