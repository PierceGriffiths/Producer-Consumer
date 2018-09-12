//the "target"s in both of these structures will, in the instances of these structs which exist at runtime, both point to the same location in memory
struct producer_args{
    unsigned num_produced, *useProducerLogPtr, *target;
    FILE *producerLog;
};

struct consumer_args{
    unsigned num_consumed, *useConsumerLogPtr, *target;
    FILE *consumerLog;
};
