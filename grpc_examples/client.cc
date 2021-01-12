#include "information.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <vector>
#include <string>
#include <thread>

class InfoClient {
public:
    InfoClient(std::shared_ptr<grpc::Channel> channel)
            :stub_(info::Manager::NewStub(channel)) {};
    void AddRecord(std::string name, int age, std::string email) {
        info::Person person;
        setPerson(person, name, age, email);

        grpc::ClientContext context;
        google::protobuf::Empty response;
        auto status = stub_->AddRecord(&context, person, &response);
        //std::cout << "peer url:" << context.peer() << std::endl;
        handleStatus(status, __func__);
    }

    void DeleteRecords (const std::vector<std::string> & name_list) {
        info::ReqName request;

        grpc::ClientContext context;
        google::protobuf::Empty response;
        std::unique_ptr<grpc::ClientWriter<info::ReqName> > writer;
        writer = stub_->DeleteRecords(&context, &response);
        for (const auto & name : name_list) {
            request.set_name(name);
            if (!writer->Write(request)) {
                std::cout << "the stream has been closed" << std::endl;
                break;
            }
        }
        writer->WritesDone(); // half close writing from the client
        grpc::Status status =  writer->Finish();
        handleStatus(status, __func__);
    }

    void GetRecordsByAge(int min, int max) {
        info::AgeRange range;
        range.set_minimal(min);
        range.set_maximal(max);

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<info::Person>> reader(stub_->GetRecordsByAge(&context, range));

        info::Person person;
        while(reader->Read(&person)) {
            std::cout << "get person:" << std::endl;
            printPerson(person);
        }
        grpc::Status status = reader->Finish();
        handleStatus(status, __func__);
    }

    void GetRecordsByNames(const std::vector<std::string> & name_list) {
        grpc::ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<info::ReqName, info::Person>> stream(stub_->GetRecordsByNames(&context)); //need to be shared_ptr since it is used by differente thread, shared_ptr is no thread-safe, but google's doc says steams's write is thread-safe with respect to read, vise versa
        std::thread worker([stream, &name_list]() {
                                info::ReqName request;
                                for (const auto & name : name_list) {
                                    request.set_name(name);
                                    if (!stream->Write(request)) {
                                        std::cout << "the stream has be closed" << std::endl;
                                        break;
                                    } else {
                                        std::cout << "send message: get " << name << std::endl;
                                    }
                                }
                                stream->WritesDone();
                           });

        info::Person person;
        while (stream->Read(&person)) {
            std::cout << "get person" << std::endl;
            printPerson(person);
        }

        worker.join();
        grpc::Status status = stream->Finish();
        handleStatus(status, __func__);
    }

private:
    void setPerson(info::Person & person, const std::string & name,
                   int age, const std::string & email) {
        person.set_name(name);
        person.set_age(age);
        person.set_email(email);
    }

    void printPerson(const info::Person & person) {
        std::cout << "Name: " << person.name()
                  << " age: " << person.age()
                  << " Email:" << person.email() << std::endl;
    }

    void handleStatus(grpc::Status status, const std::string & message) {
        if (status.ok()) {
            std::cout << message << " success" << std::endl;
        } else {
            std::cout << message
                      << " error code:   " << status.error_code()
                      << " error mesage:" << status.error_message()
                      << std::endl;
        }
    }

    std::unique_ptr<info::Manager::Stub> stub_;
};


int main() {
    InfoClient client(grpc::CreateChannel("localhost:30003",
                                          grpc::InsecureChannelCredentials()));
    client.AddRecord("xia", 10, "aaa");
    client.AddRecord("li",  20, "bbb");
    client.AddRecord("wang", 30, "ccc");

    std::vector<std::string> name_list;
    name_list.emplace_back("xia");
    client.DeleteRecords(name_list);

    client.GetRecordsByAge(20, 30);

    name_list.clear();
    name_list.emplace_back("li");
    name_list.emplace_back("wang");
    client.GetRecordsByNames(name_list);

    return 0;
}
