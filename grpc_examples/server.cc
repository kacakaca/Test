#include "information.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <vector>
#include <mutex>
#include <thread>
#include <algorithm>

class InfoServiceImpl final : public info::Manager::Service {
public:
    grpc::Status AddRecord(grpc::ServerContext* context,
                           const info::Person* person,
                           google::protobuf::Empty* /*response*/) override {
        addPerson(*person);
        return grpc::Status::OK;
    }

    grpc::Status DeleteRecords(grpc::ServerContext* context,
                               grpc::ServerReader<info::ReqName>* reader,
                               google::protobuf::Empty* /*response*/) override {
        info::ReqName request;
        while(reader->Read(&request)) {
            auto name = request.name();
            deletePerson(name);
        }
        return grpc::Status::OK;
    }

    grpc::Status GetRecordsByAge(grpc::ServerContext * context,
                                 const info::AgeRange * range,
                                 grpc::ServerWriter<info::Person> * writer) override {
        int min = range->minimal();
        int max = range->maximal();

        for(const auto & person : person_list_) {
            auto age = person.age();
            if (age >= min && age <= max ) {
                writer->Write(person);
            }
        }
        return grpc::Status::OK;
    }

    grpc::Status GetRecordsByNames(grpc::ServerContext * context,
                                   grpc::ServerReaderWriter<info::Person, info::ReqName> * stream) override {
        info::ReqName request;
        while(stream->Read(&request)) {
            auto name = request.name();
            for (const auto & person : person_list_) {
                if (person.name() == name) {
                    stream->Write(person);
                } 
            }
        }
        return grpc::Status::OK;
    }

private:
    void addPerson(const info::Person & person) {
        std::lock_guard<std::mutex> lock(person_list_mutex_);
        auto iter = std::find_if(person_list_.begin(),
                                 person_list_.end(),
                                 [&person](const info::Person & internal) -> bool {
                                     return person.name() == internal.name();
                                 });

        if (iter == person_list_.end()) {
            person_list_.emplace_back(person);
        } else {
            *iter = person;
        }
    }
    void deletePerson(const std::string & name) {
        std::lock_guard<std::mutex> lock(person_list_mutex_);
        auto iter = std::find_if(person_list_.begin(),
                                 person_list_.end(),
                                 [&name](const info::Person & person) -> bool {
                                     return person.name() == name;
                                 });
        if (iter != person_list_.end()) {
            person_list_.erase(iter);
        }
    }
    std::mutex person_list_mutex_;
    std::vector<info::Person> person_list_;
};

int main()
{
    InfoServiceImpl service;
    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server;
    std::string address("0.0.0.0:30003");
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    std::cout << "server started, listening on " << address << std::endl;
    server->Wait();
}
