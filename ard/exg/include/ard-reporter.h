#pragma once

#include <QTextCursor>
#include "a-db-utils.h"

class QTextDocument;

namespace ard {
    class TexDocReportBuilder
    {
    public:
        static void mergeTopics(TOPICS_LIST& lst);
        static void mergeToFlexbox(TOPICS_LIST& lst);
        static void guiMergeKRingKeys();

    protected:
        void merge_topics(TOPICS_LIST& lst);
        void merge_to_flexbox(TOPICS_LIST& lst);
        void merge_KRingKeys();

        void add_line();
        void add_html(QString html);
        void add_break();
        void add_title(topic_ptr f);
        void add_note(ard::note_ext* n);
        void add_text(QString txt, bool bold = false);
        void add_space();
        topic_ptr make_report(QString title);

    protected:
        QTextCursor         m_wrk_cursor;
        QTextDocument*      m_wrk_doc{nullptr};
    };
};