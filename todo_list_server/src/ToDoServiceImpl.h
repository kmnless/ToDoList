#pragma once

#include "../generated/todo.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <mutex>
#include <vector>
#include <deque>

class TodoServiceImpl final : public todo::TodoService::Service {
public:
    TodoServiceImpl() = default;

    grpc::Status AddItem(grpc::ServerContext* context,
        const todo::AddItemRequest* request,
        todo::AddItemResponse* response) override;

    grpc::Status UpdateStatus(grpc::ServerContext* context,
        const todo::UpdateStatusRequest* request,
        todo::UpdateStatusResponse* response) override;

    grpc::Status DeleteItem(grpc::ServerContext* context,
        const todo::DeleteItemRequest* request,
        todo::DeleteItemResponse* response) override;

    grpc::Status GetList(grpc::ServerContext* context,
        const todo::GetListRequest* request,
        todo::GetListResponse* response) override;

    grpc::Status SubscribeChanges(grpc::ServerContext* context,
        const todo::SubscribeRequest* request,
        grpc::ServerWriter<todo::ChangeNotification>* writer) override;

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<todo::ChangeNotification> notification_queue_;
    std::vector<std::pair<grpc::ServerWriter<todo::ChangeNotification>*, bool>> subscribers_;   // bool is used to mark for deletion
    std::vector<todo::TodoItem> items_;
    int next_id_ = 1;

    void NotifySubscribers(const todo::ChangeNotification& notification);
};