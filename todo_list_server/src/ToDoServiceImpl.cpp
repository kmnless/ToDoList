#include "ToDoServiceImpl.h"
#include <algorithm>
#include <chrono>

grpc::Status TodoServiceImpl::AddItem(grpc::ServerContext* context, const todo::AddItemRequest* request, todo::AddItemResponse* response)
{
    std::lock_guard<std::mutex> lock(mutex_);

    todo::TodoItem item;
    item.set_id(next_id_++);
    item.set_description(request->description());
    item.set_status(todo::Status::PENDING);

    items_.push_back(item);

    *response->mutable_item() = item;

    todo::ChangeNotification notification;
    notification.set_type(todo::ChangeType::ADD);
    notification.mutable_item()->CopyFrom(item);
    NotifySubscribers(notification);

    return grpc::Status::OK;
}

grpc::Status TodoServiceImpl::UpdateStatus(grpc::ServerContext* context, const todo::UpdateStatusRequest* request, todo::UpdateStatusResponse* response)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& item : items_)
    {
        if (item.id() == request->id())
        {
            item.set_status(request->status());

            todo::ChangeNotification notification;
            notification.set_type(todo::ChangeType::UPDATE);
            notification.mutable_item()->CopyFrom(item);

            NotifySubscribers(notification);

            return grpc::Status::OK;
        }
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Item not found");
}

grpc::Status TodoServiceImpl::DeleteItem(grpc::ServerContext* context, const todo::DeleteItemRequest* request, todo::DeleteItemResponse* response)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const auto& item) { return item.id() == request->id(); });

    if (it != items_.end())
    {
        todo::ChangeNotification notification;
        notification.set_type(todo::ChangeType::REMOVE);
        notification.set_removed_item_id(it->id());

        items_.erase(it);
        NotifySubscribers(notification);
        return grpc::Status::OK;
    }

    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Item not found");
}

grpc::Status TodoServiceImpl::GetList(grpc::ServerContext* context, const todo::GetListRequest* request, todo::GetListResponse* response)
{
    std::lock_guard<std::mutex> lock(mutex_);
    response->mutable_items()->Reserve(items_.size());
    for (const auto& item : items_)
    {
        *response->mutable_items()->Add() = item;
    }
    return grpc::Status::OK;
}

grpc::Status TodoServiceImpl::SubscribeChanges(grpc::ServerContext* context, const todo::SubscribeRequest*, grpc::ServerWriter<todo::ChangeNotification>* writer)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.emplace_back(writer, false);
    }

    while (!context->IsCancelled())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return context->IsCancelled() || !notification_queue_.empty(); });

        if (context->IsCancelled()) break;

        auto notification = std::move(notification_queue_.front());
        notification_queue_.pop_front();

        lock.unlock();

        if (!writer->Write(notification))
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& sub : subscribers_)
                {
                    if (sub.first == writer)
                    {
                        sub.second = true;
                        break;
                    }
                }
            }
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& sub : subscribers_)
        {
            if (sub.first == writer)
            {
                sub.second = true;
                break;
            }
        }
    }

    return grpc::Status::OK;
}

void TodoServiceImpl::NotifySubscribers(const todo::ChangeNotification& notification)
{
    std::lock_guard<std::mutex> lock(mutex_);

    notification_queue_.push_back(notification);

    subscribers_.erase(
        std::remove_if(subscribers_.begin(), subscribers_.end(),
            [](const auto& sub) { return sub.second; }),
        subscribers_.end()
    );

    cv_.notify_all();
}